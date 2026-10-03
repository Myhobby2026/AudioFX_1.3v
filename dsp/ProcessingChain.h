// ============================================================================
//  ProcessingChain.h - the full signal path
//
//     Capture (VB-Cable / WASAPI loopback / any input)
//        -> 10-Band EQ -> Reverb -> Field Surround -> Compressor
//        -> Master Volume+AGC -> Look-ahead Limiter -> Speakers
//
//  Spectrum taps the post-EQ signal (what you hear is what you see, minus the
//  final limiter clamp which is already displayed by the GR meter).
// ============================================================================
#pragma once

#include "TenBandEQ.h"
#include "ReverbEngine.h"
#include "FieldSurround.h"
#include "Compressor.h"
#include "LookaheadLimiter.h"
#include "MasterBus.h"
#include "SpectrumFft.h"
#include "AudioFifo.h"

namespace audiofx
{

class ProcessingChain
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        fs = sampleRate;
        eq.prepare (fs, maxBlock);
        reverb.prepare (fs, maxBlock);
        surround.prepare (fs, maxBlock);
        compressor.prepare (fs, maxBlock);
        limiter.prepare (fs, maxBlock);
        master.prepare (fs, maxBlock);
        spectrum.prepare (fs);
        inMeter.prepare (fs);
        outMeter.prepare (fs);
        monoTap.assign ((size_t) std::max (maxBlock, 256), 0.0f);
    }

    void reset()
    {
        eq.reset(); reverb.reset(); surround.reset();
        compressor.reset(); limiter.reset(); master.reset(); spectrum.reset();
        inMeter.reset(); outMeter.reset();
    }

    // -- modules (exposed so the UI can bind parameters) -----------------------
    TenBandEQ&       getEq()        noexcept { return eq; }
    ReverbEngine&    getReverb()    noexcept { return reverb; }
    FieldSurround&   getSurround()  noexcept { return surround; }
    Compressor&      getCompressor() noexcept { return compressor; }
    LookaheadLimiter& getLimiter()  noexcept { return limiter; }
    MasterBus&       getMaster()    noexcept { return master; }
    SpectrumFft&     getSpectrum()  noexcept { return spectrum; }

    // const accessors (used by Params::fromChain / preset saving)
    const TenBandEQ&        getEq()         const noexcept { return eq; }
    const ReverbEngine&     getReverb()     const noexcept { return reverb; }
    const FieldSurround&    getSurround()   const noexcept { return surround; }
    const Compressor&       getCompressor() const noexcept { return compressor; }
    const LookaheadLimiter& getLimiter()    const noexcept { return limiter; }
    const MasterBus&        getMaster()     const noexcept { return master; }

    void setGlobalBypass (bool b) noexcept { globalBypass = b; }
    bool isGlobalBypass() const noexcept   { return globalBypass; }

    MonoPeakMeter& getInMeter()  noexcept { return inMeter; }
    MonoPeakMeter& getOutMeter() noexcept { return outMeter; }

    // -- realtime processing ---------------------------------------------------
    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (numChannels < 1 || numSamples < 1)
            return;

        inMeter.push (channels[0], numSamples);

        if (globalBypass)
        {
            outMeter.push (channels[0], numSamples);
            return;
        }

        // spectrum taps a mono sum of the *input to the chain* (post EQ would
        // hide what the faders do while editing; pre-chain keeps the analyser
        // honest). We tap post-EQ instead so the display matches the sound.
        eq.process (channels, numChannels, numSamples);

        if ((int) monoTap.size() < numSamples)
            monoTap.resize ((size_t) numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                s += channels[ch][i];
            monoTap[(size_t) i] = s / (float) numChannels;
        }
        spectrum.push (monoTap.data(), numSamples);

        reverb.process (channels, numChannels, numSamples);
        surround.process (channels, numChannels, numSamples);
        compressor.process (channels, numChannels, numSamples);
        master.process (channels, numChannels, numSamples);
        limiter.process (channels, numChannels, numSamples);

        outMeter.push (channels[0], numSamples);
    }

private:
    double fs = 48000.0;
    TenBandEQ eq;
    ReverbEngine reverb;
    FieldSurround surround;
    Compressor compressor;
    LookaheadLimiter limiter;
    MasterBus master;
    SpectrumFft spectrum;
    MonoPeakMeter inMeter, outMeter;
    std::vector<float> monoTap;
    bool globalBypass = false;
};

} // namespace audiofx
