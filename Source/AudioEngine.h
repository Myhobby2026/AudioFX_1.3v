// ============================================================================
//  AudioEngine.h - bridges JUCE device I/O to the audiofx DSP core
//
//  Capture modes:
//    Device        - any input device (VB-Cable "CABLE Output" is the intended
//                    system-wide source: Windows plays into "CABLE Input",
//                    AudioFX captures "CABLE Output") - processed inline with
//                    minimum latency.
//    WasapiLoopback- raw WASAPI loopback of the default render endpoint
//                    ("what you hear" without VB-Cable) - captured on its own
//                    thread and rate-servoed into the render callback.
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "dsp/ProcessingChain.h"
#include "dsp/AudioFifo.h"
#include "WasapiLoopbackCapturer.h"

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    enum class CaptureMode { Device = 0, WasapiLoopback = 1 };

    AudioEngine();
    ~AudioEngine() override;

    // ---- lifecycle -----------------------------------------------------------
    void initialise();
    void shutdown();

    // ---- configuration (message thread) --------------------------------------
    void setCaptureMode (CaptureMode mode);
    CaptureMode getCaptureMode() const noexcept { return mode.load(); }

    void setInputDevice (const juce::String& name);
    void setOutputDevice (const juce::String& name);

    juce::String getInputDevice() const;
    juce::String getOutputDevice() const;

    juce::StringArray availableInputDevices() const;
    juce::StringArray availableOutputDevices() const;

    // name of the VB-Cable capture endpoint if present (else empty)
    juce::String findVBCableInput() const;

    double getCurrentSampleRate() const noexcept { return currentSampleRate.load(); }
    int    getCurrentBlockSize() const noexcept  { return currentBlockSize.load(); }
    bool   isStreaming() const noexcept          { return streaming.load(); }

    // ---- DSP -----------------------------------------------------------------
    audiofx::ProcessingChain& getChain() noexcept { return chain; }
    const audiofx::ProcessingChain& getChain() const noexcept { return chain; }

    // ---- AudioIODeviceCallback -----------------------------------------------
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;

    // called from the WASAPI loopback thread
    void pushCapture (const float* left, const float* right, int numSamples, double sourceRate);

private:
    void applyDeviceSetup();
    void startLoopback();
    void stopLoopback();

    juce::AudioDeviceManager deviceManager;
    audiofx::ProcessingChain chain;
    audiofx::AdaptiveStereoFifo captureFifo;
    std::unique_ptr<WasapiLoopbackCapturer> loopbackCapturer;

    std::atomic<CaptureMode> mode { CaptureMode::Device };
    std::atomic<bool> streaming { false };
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<int> currentBlockSize { 512 };
    std::atomic<double> loopbackRate { 48000.0 };
    std::atomic<bool> prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
