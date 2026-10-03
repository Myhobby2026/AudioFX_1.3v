// ============================================================================
//  MasterBus.h - master volume + auto level control ("maintain output level")
//
//  * manual master gain (smoothed, -60 .. +12 dB)
//  * AGC / leveler: slow RMS servo that pins long-term loudness to a target
//    dBFS, bounded to +/-12 dB of correction so it never fights the mix
// ============================================================================
#pragma once

#include "DspCommon.h"

namespace audiofx
{

class MasterBus
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        fs = sampleRate;
        volSmooth.setSampleRate (fs);
        volSmooth.setTimeConstant (0.030);
        volSmooth.snap (decibelsToGain ((double) volumeDb));

        envFast   = std::exp (-1.0 / (fs * 0.050));    //  50 ms level rise
        envSlow   = std::exp (-1.0 / (fs * 0.500));    // 500 ms level fall
        agcDown   = std::exp (-1.0 / (fs * 0.800));    // turn down: 800 ms
        agcUp     = std::exp (-1.0 / (fs * 2.500));    // turn up:   2.5 s (no pumping)
    }

    void reset() { rmsEnv = 0.0; agcGain = 1.0; }

    void setVolumeDb (float dB) noexcept  { volumeDb = clampf (dB, -60.0f, 12.0f); volSmooth.setTarget (decibelsToGain ((double) volumeDb)); }
    float getVolumeDb() const noexcept    { return volumeDb; }

    void setAgcEnabled (bool e) noexcept  { agcEnabled = e; }
    bool isAgcEnabled() const noexcept    { return agcEnabled; }

    void setAgcTargetDb (float dB) noexcept { agcTargetDb = clampf (dB, -36.0f, -6.0f); }
    float getAgcTargetDb() const noexcept   { return agcTargetDb; }

    float getProgrammeDb() const noexcept { return gainToDecibels ((float) rmsEnv + 1.0e-9f); }
    float getAgcGainDb() const noexcept   { return gainToDecibels ((float) agcGain + 1.0e-9f); }

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (numChannels < 1)
            return;

        const double target = decibelsToGain ((double) agcTargetDb);

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- level detection (energy average across channels) -------------
            double acc = 0.0;
            for (int ch = 0; ch < numChannels; ++ch)
                acc += (double) channels[ch][i] * (double) channels[ch][i];
            const double mag = std::sqrt (acc / std::max (1, numChannels));
            const double coeff = mag > rmsEnv ? envFast : envSlow;
            rmsEnv = mag + (rmsEnv - mag) * coeff;

            // ---- AGC servo ----------------------------------------------------
            if (agcEnabled && rmsEnv > 2.0e-5)
            {
                const double want = target / rmsEnv;
                const double bounded = clampd (want, kMinAgc, kMaxAgc);
                const double c = bounded < agcGain ? agcDown : agcUp;
                agcGain = bounded + (agcGain - bounded) * c;
            }
            else if (! agcEnabled)
            {
                agcGain = 1.0 + (agcGain - 1.0) * agcUp;
            }

            const float gv = (float) (volSmooth.next() * agcGain);
            for (int ch = 0; ch < numChannels; ++ch)
                channels[ch][i] *= gv;
        }
    }

    static constexpr double kMinAgc = 0.251;   // -12 dB
    static constexpr double kMaxAgc = 3.981;   // +12 dB

private:
    double fs = 48000.0;
    Smoothed volSmooth;
    float volumeDb = -6.0f;
    float agcTargetDb = -18.0f;
    bool agcEnabled = true;
    double agcGain = 1.0, rmsEnv = 0.0;
    double envFast = 0.9, envSlow = 0.99, agcDown = 0.99, agcUp = 0.999;
};

} // namespace audiofx
