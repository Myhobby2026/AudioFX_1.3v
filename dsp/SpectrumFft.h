// ============================================================================
//  SpectrumFft.h - dependency-free radix-2 FFT spectrum analyser
//  Push audio, pull a smoothed dB spectrum (log-frequency mapping left to UI).
// ============================================================================
#pragma once

#include "DspCommon.h"
#include <array>
#include <complex>
#include <vector>

namespace audiofx
{

class SpectrumFft
{
public:
    static constexpr int kFftSize = 4096;          // ~85 ms @ 48 kHz
    static constexpr int kNumBins = kFftSize / 2;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        window.resize (kFftSize);
        for (int i = 0; i < kFftSize; ++i)
            window[(size_t) i] = 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) (kFftSize - 1));   // Hann

        fifo.assign (kFftSize, 0.0f);
        writePos = 0;
        fillCount = 0;
        mag.assign (kNumBins, 0.0f);
        peakHold.assign (kNumBins, -120.0f);
        scratch.assign (kFftSize, {});
    }

    void reset()
    {
        std::fill (fifo.begin(), fifo.end(), 0.0f);
        std::fill (mag.begin(), mag.end(), 0.0f);
        std::fill (peakHold.begin(), peakHold.end(), -120.0f);
        writePos = 0; fillCount = 0;
    }

    // push mixed-down mono samples; performs an FFT hop every kFftSize/2 samples
    void push (const float* samples, int numSamples, float peakDecayDbPerSec = 24.0f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            fifo[(size_t) writePos] = samples[i];
            writePos = (writePos + 1) % kFftSize;
            if (++fillCount >= kFftSize / 2)
            {
                fillCount = 0;
                runFft();
            }
        }

        // peak-hold decay
        const float dt = (float) numSamples / (float) (fs > 0 ? fs : 48000.0);
        const float dec = peakDecayDbPerSec * dt;
        for (int b = 0; b < kNumBins; ++b)
            peakHold[(size_t) b] -= dec;
    }

    // dB magnitudes (bins 0 .. kNumBins-1), already smoothed
    const std::vector<float>& getMagnitudesDb() const noexcept { return mag; }
    const std::vector<float>& getPeakHoldDb()   const noexcept { return peakHold; }

    double binToFrequency (int bin) const noexcept { return (double) bin * fs / (double) kFftSize; }
    double frequencyToBin (double freq) const noexcept { return freq * (double) kFftSize / fs; }

private:
    void runFft()
    {
        // gather windowed frame in time order (oldest first)
        const int start = writePos;
        for (int i = 0; i < kFftSize; ++i)
        {
            const float s = fifo[(size_t) ((start + i) % kFftSize)];
            scratch[(size_t) i] = std::complex<double> ((double) s * window[(size_t) i], 0.0);
        }
        fft (scratch);

        const double norm = 2.0 / (double) kFftSize;
        for (int b = 0; b < kNumBins; ++b)
        {
            const double m = std::abs (scratch[(size_t) b]) * norm;
            const float dB = (float) clampd (gainToDecibels ((float) m), -120.0, 12.0);
            // exponential smoothing for a fluid display
            mag[(size_t) b] = mag[(size_t) b] * 0.55f + dB * 0.45f;
            peakHold[(size_t) b] = std::max (peakHold[(size_t) b], dB);
        }
    }

    static void fft (std::vector<std::complex<double>>& a)
    {
        const size_t n = a.size();
        // bit reversal
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap (a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = -2.0 * kPi / (double) len;
            const std::complex<double> wlen (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1.0, 0.0);
                for (size_t k = 0; k < len / 2; ++k)
                {
                    const std::complex<double> u = a[i + k];
                    const std::complex<double> v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
    }

    double fs = 48000.0;
    std::vector<float> window, fifo, mag, peakHold;
    std::vector<std::complex<double>> scratch;
    int writePos = 0, fillCount = 0;
};

} // namespace audiofx
