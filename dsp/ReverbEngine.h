// ============================================================================
//  ReverbEngine.h - Freeverb-style Schroeder/Moorer reverb (8 combs + 4 allpass
//  per channel, stereo spread). Tunings scale with the engine sample rate.
// ============================================================================
#pragma once

#include "DspCommon.h"
#include <array>

namespace audiofx
{

class ReverbEngine
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        fs = sampleRate;
        const double scale = fs / 44100.0;

        static const int combTuning[8]    = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int allpassTuning[4] = { 556, 441, 341, 225 };

        for (int ch = 0; ch < 2; ++ch)
        {
            const int spread = ch == 0 ? 0 : 23;
            for (int i = 0; i < 8; ++i)
                combs[i][ch].resize ((int) (combTuning[i] * scale) + spread);
            for (int i = 0; i < 4; ++i)
                allpasses[i][ch].resize ((int) (allpassTuning[i] * scale) + spread);
        }
        reset();
        updateCoeffs();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 8; ++i)
                combs[i][ch].reset();
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 4; ++i)
                allpasses[i][ch].reset();
    }

    // --- parameters -----------------------------------------------------------
    void setRoomSize (float v) noexcept  { roomSize = clampf (v, 0.0f, 1.0f); updateCoeffs(); }
    void setDamping  (float v) noexcept  { damping  = clampf (v, 0.0f, 1.0f); updateCoeffs(); }
    void setWetLevel (float v) noexcept  { wetLevel = clampf (v, 0.0f, 1.0f); }
    void setDryLevel (float v) noexcept  { dryLevel = clampf (v, 0.0f, 1.0f); }
    void setWidth    (float v) noexcept  { width    = clampf (v, 0.0f, 1.0f); }
    void setFreeze   (bool f)  noexcept  { freeze = f; updateCoeffs(); }
    void setBypassed (bool b)  noexcept  { bypassed = b; }

    float getRoomSize() const noexcept { return roomSize; }
    float getDamping()  const noexcept { return damping; }
    float getWetLevel() const noexcept { return wetLevel; }
    float getDryLevel() const noexcept { return dryLevel; }
    float getWidth()    const noexcept { return width; }
    bool  getFreeze()   const noexcept { return freeze; }
    bool  isBypassed()  const noexcept { return bypassed; }

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (bypassed || numChannels < 1)
            return;

        float* L = channels[0];
        float* R = numChannels > 1 ? channels[1] : channels[0];

        const double w1 = (double) wetLevel * ((double) width * 0.5 + 0.5);
        const double w2 = (double) wetLevel * ((1.0 - (double) width) * 0.5);
        const double dry = (double) dryLevel;

        for (int i = 0; i < numSamples; ++i)
        {
            const double inL = L[i], inR = R[i];
            // mono input to the tank keeps the image centred
            const double input = (inL + inR) * 0.5 * inputGain;

            double outL = 0.0, outR = 0.0;
            for (int c = 0; c < 8; ++c)
            {
                outL += combs[c][0].process (input);
                outR += combs[c][1].process (input);
            }
            for (int a = 0; a < 4; ++a)
            {
                outL = allpasses[a][0].process (outL);
                outR = allpasses[a][1].process (outR);
            }

            const double yL = inL * dry + w1 * outL + w2 * outR;
            const double yR = inR * dry + w2 * outL + w1 * outR;
            L[i] = (float) yL;
            if (numChannels > 1)
                R[i] = (float) yR;
        }
    }

    static constexpr float kRangeDb = 15.0f;

private:
    struct Comb
    {
        void resize (int n) { buf.assign ((size_t) std::max (8, n), 0.0f); idx = 0; store = 0.0f; }
        void reset() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; store = 0.0f; }
        float process (double input) noexcept
        {
            if (buf.empty()) return 0.0f;
            const double out = buf[(size_t) idx];
            store = out * (1.0 - damp2) + store * damp2;        // damping lowpass in feedback
            buf[(size_t) idx] = (float) (input + store * feedback);
            if (++idx >= (int) buf.size()) idx = 0;
            return (float) out;
        }
        std::vector<float> buf;
        int idx = 0;
        double feedback = 0.84, damp2 = 0.2, store = 0.0;
    };

    struct Allpass
    {
        void resize (int n) { buf.assign ((size_t) std::max (8, n), 0.0f); idx = 0; }
        void reset() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }
        // True Schroeder allpass: H(z) = (z^-d - g) / (1 - g z^-d), |H| == 1
        float process (double input) noexcept
        {
            if (buf.empty()) return (float) input;
            const double bufout = buf[(size_t) idx];
            const double y = bufout - feedbackGain * input;
            buf[(size_t) idx] = (float) (input + feedbackGain * y);
            if (++idx >= (int) buf.size()) idx = 0;
            return (float) y;
        }
        std::vector<float> buf;
        int idx = 0;
        static constexpr double feedbackGain = 0.5;
    };

    void updateCoeffs()
    {
        // Freeverb mapping: roomsize 0..1 -> feedback 0.70..0.98 (freeze locks at 1.0)
        const double fb = freeze ? 1.0 : 0.70 + 0.28 * (double) roomSize;
        const double damp = freeze ? 0.0 : 0.10 + 0.80 * (double) damping;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 8; ++i)
            {
                combs[i][ch].feedback = fb;
                combs[i][ch].damp2 = damp * 0.4;   // map to one-pole coefficient range
            }
    }

    double fs = 48000.0;
    Comb combs[8][2];
    Allpass allpasses[4][2];

    float roomSize = 0.55f, damping = 0.40f, wetLevel = 0.22f, dryLevel = 1.0f, width = 1.0f;
    bool freeze = false, bypassed = false;
    static constexpr double inputGain = 0.22;   // keeps the tank well below full scale
};

} // namespace audiofx
