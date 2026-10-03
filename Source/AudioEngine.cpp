// ============================================================================
//  AudioEngine.cpp
// ============================================================================
#include "AudioEngine.h"
#include "State/Params.h"

AudioEngine::AudioEngine()
{
    captureFifo.prepare (48000.0, 48000 / 2);
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::initialise()
{
    const auto saved = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                           .getChildFile ("AudioFX")
                           .getChildFile ("devices.xml");

    std::unique_ptr<juce::XmlElement> xml;
    if (saved.existsAsFile())
        xml = juce::parseXML (saved.loadFileAsString());

    deviceManager.initialise (2, 2, xml.get(), true);
    deviceManager.addAudioCallback (this);
}

void AudioEngine::shutdown()
{
    stopLoopback();
    deviceManager.removeAudioCallback (this);

    const auto saved = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                           .getChildFile ("AudioFX")
                           .getChildFile ("devices.xml");
    saved.getParentDirectory().createDirectory();
    if (auto xml = deviceManager.createStateXml())
        saved.replaceWithText (xml->toString());
}

// ----------------------------------------------------------------------------
void AudioEngine::setCaptureMode (CaptureMode m)
{
    if (mode.load() == m)
        return;

    mode = m;
    if (m == CaptureMode::WasapiLoopback)
    {
        // leave the input side free; loopback thread feeds the FIFO
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup (setup);
        setup.inputDeviceName = {};
        setup.useDefaultInputChannels = false;
        deviceManager.setAudioDeviceSetup (setup, true);
        startLoopback();
    }
    else
    {
        stopLoopback();
        applyDeviceSetup();
    }
}

void AudioEngine::setInputDevice (const juce::String& name)
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.inputDeviceName = name;
    setup.useDefaultInputChannels = name.isNotEmpty();
    deviceManager.setAudioDeviceSetup (setup, true);
}

void AudioEngine::setOutputDevice (const juce::String& name)
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.outputDeviceName = name;
    setup.useDefaultOutputChannels = true;
    deviceManager.setAudioDeviceSetup (setup, true);
}

juce::String AudioEngine::getInputDevice() const
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    return setup.inputDeviceName;
}

juce::String AudioEngine::getOutputDevice() const
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    return setup.outputDeviceName;
}

// JUCE 8: device lists live on the AudioIODeviceType, not on AudioDeviceManager.
// (getAvailableDeviceTypes() is non-const in JUCE 8, hence the const_cast.)
// getDeviceNames(false) = render/playback names, getDeviceNames(true) = capture names.
static juce::StringArray namesFromTypes (const juce::AudioDeviceManager& dm, bool inputs)
{
    juce::StringArray names;
    for (auto* t : const_cast<juce::AudioDeviceManager&> (dm).getAvailableDeviceTypes())
        for (const auto& n : t->getDeviceNames (inputs))
            names.addIfNotAlreadyThere (n);
    return names;
}

juce::StringArray AudioEngine::availableInputDevices() const  { return namesFromTypes (deviceManager, true); }
juce::StringArray AudioEngine::availableOutputDevices() const { return namesFromTypes (deviceManager, false); }

juce::String AudioEngine::findVBCableInput() const
{
    for (const auto& d : availableInputDevices())
        if (d.containsIgnoreCase ("CABLE Output") || d.containsIgnoreCase ("VB-Audio"))
            return d;
    return {};
}

void AudioEngine::applyDeviceSetup()
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.useDefaultInputChannels = setup.inputDeviceName.isNotEmpty();
    deviceManager.setAudioDeviceSetup (setup, true);
}

void AudioEngine::startLoopback()
{
    stopLoopback();
    loopbackCapturer = std::make_unique<WasapiLoopbackCapturer>();
    loopbackCapturer->start ([this] (const float* l, const float* r, int n, double sr)
    {
        pushCapture (l, r, n, sr);
    });
}

void AudioEngine::stopLoopback()
{
    if (loopbackCapturer)
        loopbackCapturer->stop();
    loopbackCapturer = nullptr;
}

void AudioEngine::pushCapture (const float* left, const float* right, int numSamples, double sourceRate)
{
    if (std::abs (loopbackRate.load() - sourceRate) > 0.5)
    {
        captureFifo.setSourceRate (sourceRate);
        loopbackRate = sourceRate;
    }
    captureFifo.push (left, right, numSamples);
}

// ----------------------------------------------------------------------------
void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    const double sr   = device != nullptr ? device->getCurrentSampleRate() : 48000.0;
    const int block   = device != nullptr ? juce::jmax (64, device->getCurrentBufferSizeSamples()) : 512;

    currentSampleRate = sr;
    currentBlockSize  = block;

    chain.prepare (sr, juce::jmax (block, 256));
    captureFifo.prepare (sr, (int) (sr * 0.5));          // 500 ms of capture headroom
    captureFifo.setDestRate (sr);
    if (mode.load() == CaptureMode::WasapiLoopback)
        captureFifo.setSourceRate (loopbackRate.load());

    prepared = true;
    streaming = true;
}

void AudioEngine::audioDeviceStopped()
{
    streaming = false;
    prepared = false;
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext&)
{
    if (numOutputChannels < 1 || outputChannelData == nullptr || outputChannelData[0] == nullptr)
        return;

    float* outL = outputChannelData[0];
    float* outR = numOutputChannels > 1 ? outputChannelData[1] : nullptr;

    // ---- fetch captured audio ------------------------------------------------
    if (mode.load() == CaptureMode::Device)
    {
        if (numInputChannels > 0 && inputChannelData != nullptr && inputChannelData[0] != nullptr)
        {
            const float* inL = inputChannelData[0];
            const float* inR = numInputChannels > 1 ? inputChannelData[1] : inL;
            if (inR == nullptr) inR = inL;

            juce::FloatVectorOperations::copy (outL, inL, numSamples);
            if (outR != nullptr)
                juce::FloatVectorOperations::copy (outR, inR, numSamples);
        }
        else
        {
            juce::FloatVectorOperations::clear (outL, numSamples);
            if (outR != nullptr)
                juce::FloatVectorOperations::clear (outR, numSamples);
        }
    }
    else
    {
        captureFifo.pull (outL, outR, numSamples);
    }

    // silence unused extra outputs (5.1 cards etc.)
    for (int ch = 2; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear (outputChannelData[ch], numSamples);

    // ---- process -------------------------------------------------------------
    if (prepared.load())
    {
        float* chans[2] = { outL, outR };
        chain.process (chans, outR != nullptr ? 2 : 1, numSamples);
    }
}
