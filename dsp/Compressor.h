// ============================================================================
//  Compressor.h - programme compressor with soft knee + makeup gain
//
//  * threshold / ratio / attack / release / makeup controls
//  * 6 dB soft knee for musical, click-free transitions
//  * peak-envelope detector (fast attack, slow release)
//  * GR meter output (positive dB = amount of reduction) for the UI
//  * realtime-safe: no allocation or locks in process()
// ============================================================================
#pragma once

#include "DspCommon.h"
#include <atomic>

namespace audiofx
{

class Compressor
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        fs = sampleRate;
        updateCoeffs();
        makeupSmooth.setSampleRate (fs);
        makeupSmooth.setTimeConstant (0.030);
        makeupSmooth.snap (decibelsToGain ((double) makeupDb));
    }

    void reset() noexcept { env = 0.0; grDb = 0.0; grMeter.store (0.0f); }

    void setThresholdDb (float dB) noexcept { thresholdDb = clampf (dB, -40.0f, 0.0f); }
    float getThresholdDb() const noexcept   { return thresholdDb; }

    void setRatio (float r) noexcept        { ratio = clampf (r, 1.0f, 20.0f); }
    float getRatio() const noexcept         { return ratio; }

    void setAttackMs (float ms) noexcept    { attackMs = clampf (ms, 0.5f, 200.0f); updateCoeffs(); }
    float getAttackMs() const noexcept      { return attackMs; }

    void setReleaseMs (float ms) noexcept   { releaseMs = clampf (ms, 10.0f, 1000.0f); updateCoeffs(); }
    float getReleaseMs() const noexcept     { return releaseMs; }

    void setMakeupDb (float dB) noexcept    { makeupDb = clampf (dB, 0.0f, 24.0f); makeupSmooth.setTarget (decibelsToGain ((double) makeupDb)); }
    float getMakeupDb() const noexcept      { return makeupDb; }

    void setBypassed (bool b) noexcept      { bypassed = b; }
    bool isBypassed() const noexcept        { return bypassed; }

    // gain reduction in dB (positive = amount of reduction), for the GR meter
    float getGainReductionDb() const noexcept { return grMeter.load(); }

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (numChannels < 1 || numSamples < 1)
            return;

        if (bypassed)
        {
            grDb = 0.0;
            grMeter.store (0.0f);
            return;                                   // bit-transparent bypass
        }

        const double t = (double) thresholdDb;
        const double r = (double) ratio;
        const double halfK = kneeDb * 0.5;

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- peak detection across channels -----------------------------
            double mag = 0.0;
            for (int ch = 0; ch < numChannels; ++ch)
                mag = std::max (mag, (double) std::abs (channels[ch][i]));

            // ---- peak envelope (fast attack, slow release) ------------------
            const double ce = mag > env ? envAtk : envRel;
            env = mag + (env - mag) * ce;

            // ---- static curve with 6 dB soft knee ----------------------------
            const double xDb = gainToDecibels (env + 1.0e-9);
            double g = 0.0;                                   // dB, <= 0
            if (xDb > t + halfK)
                g = (t + (xDb - t) / r) - xDb;
            else if (xDb > t - halfK)
            {
                const double over = xDb - t + halfK;
                g = -(1.0 - 1.0 / r) * over * over / (2.0 * kneeDb);
            }

            // ---- smooth the reduction: grab in 5 ms, recover in 120 ms -------
            const double cg = g < grDb ? grAtk : grRel;
            grDb = g + (grDb - g) * cg;

            const float gv = (float) (decibelsToGain (grDb) * makeupSmooth.next());
            for (int ch = 0; ch < numChannels; ++ch)
                channels[ch][i] *= gv;
        }

        grMeter.store ((float) std::max (0.0, -grDb));
    }

private:
    void updateCoeffs()
    {
        envAtk = std::exp (-1.0 / (fs * (double) clampf (attackMs, 0.5f, 200.0f) * 0.001));
        envRel = std::exp (-1.0 / (fs * (double) clampf (releaseMs, 10.0f, 1000.0f) * 0.001));
        grAtk  = std::exp (-1.0 / (fs * 0.005));
        grRel  = std::exp (-1.0 / (fs * 0.120));
    }

    double fs = 48000.0;
    float thresholdDb = -18.0f;
    float ratio       = 3.0f;
    float attackMs    = 10.0f;
    float releaseMs   = 150.0f;
    float makeupDb    = 0.0f;
    bool  bypassed    = false;

    double env = 0.0, grDb = 0.0;
    double envAtk = 0.5, envRel = 0.999, grAtk = 0.9, grRel = 0.999;
    double kneeDb = 6.0;
    Smoothed makeupSmooth;
    std::atomic<float> grMeter { 0.0f };
};

} // namespace audiofx
