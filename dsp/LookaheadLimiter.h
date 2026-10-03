// ============================================================================
//  LookaheadLimiter.h - zero-overshoot look-ahead peak limiter
//
//  Pipeline (stereo-linked):
//    1. peak detector on the pre-gain signal
//    2. desired gain  g_c = min(1, ceiling / peak)
//    3. sliding minimum of g_c over the look-ahead window
//    4. gain smoothing: linear attack ramp of exactly W samples toward the
//       sliding minimum (full attenuation lands the moment the peak emerges
//       from the delay), one-pole release upward
//    5. apply smoothed gain to the W-sample delayed signal
//    6. final hard clamp at the ceiling guarantees the output contract
// ============================================================================
#pragma once

#include "DspCommon.h"
#include <atomic>
#include <vector>

namespace audiofx
{

class LookaheadLimiter
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        fs = sampleRate;
        lookSamples = std::max (16, (int) (fs * 0.005));           // 5 ms look-ahead
        attackSamples = (double) lookSamples;
        delayL.resize (lookSamples + 4);
        delayR.resize (lookSamples + 4);
        gainRing.assign ((size_t) (lookSamples + 2), 1.0f);
        ringPos = 0;
        rampPos = 0; rampStart = 1.0; rampTarget = 1.0; g = 1.0;
        setReleaseMs (releaseMs);
        setCeilingDb (ceilingDb);
    }

    void reset()
    {
        delayL.reset();
        delayR.reset();
        std::fill (gainRing.begin(), gainRing.end(), 1.0f);
        ringPos = 0; rampPos = 0; rampStart = 1.0; rampTarget = 1.0; g = 1.0;
        grMeter = 1.0; grMinBlock = 1.0;
    }

    void setCeilingDb (float dB) noexcept  { ceilingDb = clampf (dB, -24.0f, 0.0f); ceiling = (double) decibelsToGain (ceilingDb); }
    void setReleaseMs (float ms) noexcept  { releaseMs = clampf (ms, 5.0f, 1000.0f); releaseCoeff = std::exp (-1.0 / (fs * (double) releaseMs * 0.001)); }
    void setBypassed  (bool b) noexcept    { bypassed = b; }

    float getCeilingDb() const noexcept { return ceilingDb; }
    float getReleaseMs() const noexcept { return releaseMs; }
    bool  isBypassed()   const noexcept { return bypassed; }

    // gain reduction in dB (positive = amount of reduction), for the GR meter
    float getGainReductionDb() const noexcept { return (float) std::max (0.0, -gainToDecibels (grMeter.load())); }

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (bypassed || numChannels < 1)
            return;

        float* L = channels[0];
        float* R = numChannels > 1 ? channels[1] : nullptr;
        grMinBlock = 1.0;

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- 1. stereo-linked peak detection -------------------------------
            const double xL = L[i];
            const double xR = R != nullptr ? R[i] : xL;
            const double peak = std::max (std::abs (xL), std::abs (xR));

            // ---- 2. desired gain ----------------------------------------------
            const double d = peak > 1.0e-12 ? std::min (1.0, ceiling / peak) : 1.0;

            // ---- 3. sliding minimum over the look-ahead window ------------------
            gainRing[(size_t) ringPos] = (float) d;
            ringPos = (ringPos + 1) % (int) gainRing.size();
            double m = 1.0;
            for (float v : gainRing)
                m = std::min (m, (double) v);

            // ---- 4. gain smoothing: attack ramp + one-pole release -------------
            if (m < g - 1.0e-7)                            // need attenuation -> attack ramp
            {
                if (rampPos <= 0) { rampStart = g; rampTarget = m; rampPos = 1; }
                else              { rampTarget = std::min (rampTarget, m); ++rampPos; }
                const double t = clampd ((double) rampPos / attackSamples, 0.0, 1.0);
                g = rampStart + (rampTarget - rampStart) * t;
                if (rampPos >= (int) attackSamples) { g = rampTarget; rampPos = 0; }
            }
            else                                           // release toward unity (or toward m if still hot)
            {
                rampPos = 0;
                const double target = std::min (1.0, m);
                g = target + (g - target) * releaseCoeff;
                g = clampd (g, 1.0e-6, 1.0);
            }

            grMinBlock = std::min (grMinBlock, g);

            // ---- 5. apply to delayed signal (hard clamp = hard guarantee) -------
            delayL.write ((float) xL);
            delayR.write ((float) xR);
            L[i] = clampf (delayL.read (lookSamples) * (float) g, (float) -ceiling, (float) ceiling);
            if (R != nullptr)
                R[i] = clampf (delayR.read (lookSamples) * (float) g, (float) -ceiling, (float) ceiling);
        }

        // ---- GR meter ballistics: instant attack, ~250 ms recovery -------------
        const double mn = grMinBlock;
        double gm = grMeter.load();
        if (mn < gm) gm = mn;
        else         gm = mn + (gm - mn) * 0.93;
        grMeter = gm;
    }

private:
    double fs = 48000.0;
    DelayLine delayL, delayR;
    std::vector<float> gainRing;
    int ringPos = 0, lookSamples = 240;
    int rampPos = 0;
    double attackSamples = 240.0;
    double rampStart = 1.0, rampTarget = 1.0, g = 1.0;
    double releaseCoeff = 0.999, releaseMs = 120.0;
    double ceiling = 0.966, ceilingDb = -0.3;
    std::atomic<double> grMeter { 1.0 };
    double grMinBlock = 1.0;
    bool bypassed = false;
};

} // namespace audiofx
