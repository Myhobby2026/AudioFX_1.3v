// ============================================================================
//  TenBandEQ.h - 10-band graphic equalizer (shelving ends, peaking mids)
// ============================================================================
#pragma once

#include "DspCommon.h"
#include <array>

namespace audiofx
{

class TenBandEQ
{
public:
    static constexpr int kNumBands = 10;

    // ISO-ish octave centres (Hz)
    static const std::array<double, kNumBands>& frequencies() noexcept
    {
        static const std::array<double, kNumBands> f = { 31.25, 62.5, 125.0, 250.0, 500.0,
                                                         1000.0, 2000.0, 4000.0, 8000.0, 16000.0 };
        return f;
    }

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        fs = sampleRate;
        for (int b = 0; b < kNumBands; ++b)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                filters[b][ch].reset();
                coeffFilter[b][ch].setIdentity();
            }
            gainSmooth[b].setSampleRate (fs);
            gainSmooth[b].setTimeConstant (0.030);
            gainSmooth[b].snap (gainDb[b]);
        }
        updateCoefficients();
    }

    void reset()
    {
        for (int b = 0; b < kNumBands; ++b)
            for (int ch = 0; ch < 2; ++ch)
                filters[b][ch].reset();
    }

    void setGainDb (int band, float dB) noexcept
    {
        if (band >= 0 && band < kNumBands)
        {
            gainDb[band] = clampf (dB, -kRangeDb, kRangeDb);
            gainSmooth[band].setTarget (gainDb[band]);
        }
    }

    float getGainDb (int band) const noexcept { return (band >= 0 && band < kNumBands) ? gainDb[band] : 0.0f; }

    void setBypassed (bool b) noexcept  { bypassed = b; }
    bool isBypassed() const noexcept    { return bypassed; }

    // Magnitude of the combined response (for the GUI curve), linear gain
    double responseAt (double freq) const noexcept
    {
        double mag = 1.0;
        for (int b = 0; b < kNumBands; ++b)
            mag *= coeffFilter[b][0].magnitudeAt (fs > 0 ? fs : 48000.0, freq);
        return mag;
    }

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (bypassed)
            return;

        const int nCh = std::min (numChannels, 2);

        // Smooth gains once per block (sub-sliced for zipper-free sweeps)
        bool changed = false;
        for (int b = 0; b < kNumBands; ++b)
        {
            const double before = gainSmooth[b].getCurrent();
            gainSmooth[b].skip (numSamples);
            if (std::abs (before - gainSmooth[b].getCurrent()) > 1.0e-4)
                changed = true;
        }
        if (changed || firstBlock)
        {
            updateCoefficients();
            firstBlock = false;
        }

        for (int ch = 0; ch < nCh; ++ch)
        {
            float* d = channels[ch];
            for (int i = 0; i < numSamples; ++i)
            {
                float x = d[i];
                for (int b = 0; b < kNumBands; ++b)
                    x = filters[b][ch].process (x);
                d[i] = x;
            }
        }
    }

    static constexpr float kRangeDb = 15.0f;

private:
    void updateCoefficients()
    {
        for (int b = 0; b < kNumBands; ++b)
        {
            const double g = gainSmooth[b].getCurrent();
            for (int ch = 0; ch < 2; ++ch)
            {
                if (b == 0)                       filters[b][ch].setLowShelf  (fs, frequencies()[0], g, 0.60);
                else if (b == kNumBands - 1)      filters[b][ch].setHighShelf (fs, frequencies()[(size_t) b], g, 0.60);
                else                              filters[b][ch].setPeaking   (fs, frequencies()[(size_t) b], g, 1.0);
                coeffFilter[b][ch] = filters[b][ch];
            }
        }
    }

    double fs = 48000.0;
    std::array<float, kNumBands>  gainDb {};
    std::array<Smoothed, kNumBands> gainSmooth {};
    std::array<std::array<Biquad, 2>, kNumBands> filters {}, coeffFilter {};
    bool bypassed = false, firstBlock = true;
};

} // namespace audiofx
