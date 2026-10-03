// ============================================================================
//  FieldSurround.h - "Field Surround" stereo field expander
//   * mid/side spread with decorrelated side (allpass diffusion)
//   * Haas micro-delay widening (0 .. ~1.1 ms, scales with Amount)
//   * bass focus: progressively monoises the sub-120 Hz band so the low end
//     stays anchored at the centre of the virtual field
//
//  Phase-coherent crossover by subtraction:
//      low  = LP(wide)          high = wide - low
//      out  = wide + bassFocus * (monoLow - low)
//  so bassFocus = 0 -> fully widened, bassFocus = 1 -> mono bass, wide highs.
// ============================================================================
#pragma once

#include "DspCommon.h"

namespace audiofx
{

class FieldSurround
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        fs = sampleRate;
        haasDelay.resize ((int) (fs * 0.02) + 8);          // up to 20 ms of headroom
        sideAp1.reset(); sideAp2.reset(); sideAp3.reset();
        sideAp1.setAllpass (fs, 2100.0, 0.6);
        sideAp2.setAllpass (fs,  870.0, 0.6);
        sideAp3.setAllpass (fs,  330.0, 0.6);
        loL.setLowpass (fs, 120.0);
        loR.setLowpass (fs, 120.0);
        reset();
    }

    void reset()
    {
        haasDelay.reset();
        sideAp1.reset(); sideAp2.reset(); sideAp3.reset();
        loL.reset(); loR.reset();
    }

    void setAmount    (float v) noexcept { amount    = clampf (v, 0.0f, 1.0f); }
    void setSpread    (float v) noexcept { spread    = clampf (v, 0.0f, 2.0f); }
    void setBassFocus (float v) noexcept { bassFocus = clampf (v, 0.0f, 1.0f); }
    void setCenter    (float v) noexcept { center    = clampf (v, 0.0f, 1.5f); }
    void setBypassed  (bool b) noexcept  { bypassed = b; }

    float getAmount()    const noexcept { return amount; }
    float getSpread()    const noexcept { return spread; }
    float getBassFocus() const noexcept { return bassFocus; }
    float getCenter()    const noexcept { return center; }
    bool  isBypassed()   const noexcept { return bypassed; }

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (bypassed || numChannels < 2)
            return;

        float* L = channels[0];
        float* R = channels[1];

        const double amt = (double) amount;
        const double bf  = (double) bassFocus;
        const double haasSamples = amt * 0.0011 * fs + 1.0;   // 1..~54 samples

        for (int i = 0; i < numSamples; ++i)
        {
            const double inL = L[i], inR = R[i];

            // ---- mid / side decomposition ------------------------------------
            const double mid  = 0.5 * (inL + inR) * (double) center;
            const double side = 0.5 * (inL - inR) * (double) spread;

            // Feed a slice of mid into the side network so dual-mono / mono
            // sources also bloom into a virtual field (psychoacoustic widening),
            // then decorrelate through the diffusion allpasses.
            const double sideIn = side + mid * amt * 0.5;
            double s = sideIn;
            s = sideAp1.process ((float) s);
            s = sideAp2.process ((float) s);
            s = sideAp3.process ((float) s);
            const double sideOut = sideIn * (1.0 - amt) + s * amt;

            // ---- Haas micro-delay on the right side contribution -------------
            haasDelay.write ((float) sideOut);
            const double sideR = amt > 0.001 ? (double) haasDelay.read (haasSamples) : sideOut;

            const double wideL = mid + sideOut;
            const double wideR = mid + sideR;

            // ---- phase-coherent bass management -------------------------------
            const double lowL = (double) loL.process ((float) wideL);
            const double lowR = (double) loR.process ((float) wideR);
            const double monoLow = 0.5 * (lowL + lowR);

            L[i] = (float) (wideL + bf * (monoLow - lowL));
            R[i] = (float) (wideR + bf * (monoLow - lowR));
        }
    }

private:
    double fs = 48000.0;
    float amount = 0.35f, spread = 1.15f, bassFocus = 0.7f, center = 1.0f;
    bool bypassed = false;

    DelayLine haasDelay;
    Biquad sideAp1, sideAp2, sideAp3;
    Biquad loL, loR;
};

} // namespace audiofx
