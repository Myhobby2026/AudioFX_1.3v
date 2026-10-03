// ============================================================================
//  AudioFifo.h - capture -> render FIFO with adaptive resampler (rate servo)
//
//  Decouples the capture clock (VB-Cable / WASAPI loopback) from the render
//  clock.  A gentle proportional servo nudges the resampling ratio so the FIFO
//  stays half-full even when the two endpoints drift apart in sample rate.
// ============================================================================
#pragma once

#include "DspCommon.h"
#include <atomic>
#include <vector>

namespace audiofx
{

class AdaptiveStereoFifo
{
public:
    void prepare (double nominalRate, int capacitySamples)
    {
        srcRate = dstRate = nominalRate;
        capacity = std::max (capacitySamples, 256);
        bufL.assign ((size_t) capacity, 0.0f);
        bufR.assign ((size_t) capacity, 0.0f);
        readPos = writePos = used = 0;
        ratio = 1.0;
        fracPos = 0.0;
        targetFill = capacity / 2;
    }

    void setSourceRate (double sr) noexcept
    {
        srcRate = sr > 0 ? sr : 48000.0;
        ratio = srcRate / dstRate;
        fracPos = 0.0;
    }

    void setDestRate (double sr) noexcept { dstRate = sr > 0 ? sr : 48000.0; ratio = srcRate / dstRate; }

    void reset()
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        readPos = writePos = used = 0;
        fracPos = 0.0;
    }

    // push a captured block (stereo; r may equal l for mono sources)
    void push (const float* l, const float* r, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            if (used >= capacity - 2)   // overflow: drop sample
                continue;
            bufL[(size_t) writePos] = l[i];
            bufR[(size_t) writePos] = r != nullptr ? r[i] : l[i];
            writePos = (writePos + 1) % capacity;
            ++used;
        }
        updateServo();
    }

    // pull a rendered block at the destination rate; returns false on underrun
    bool pull (float* l, float* r, int n)
    {
        bool ok = true;
        for (int i = 0; i < n; ++i)
        {
            if (used < 2)
            {
                l[i] = 0.0f; if (r != nullptr) r[i] = 0.0f;
                ok = false;
                continue;
            }
            // linear interpolation read at fracPos across the two oldest samples
            const float aL = bufL[(size_t) readPos], bL = bufL[(size_t) ((readPos + 1) % capacity)];
            const float aR = bufR[(size_t) readPos], bR = bufR[(size_t) ((readPos + 1) % capacity)];
            const float f = (float) fracPos;
            l[i] = aL + (bL - aL) * f;
            if (r != nullptr)
                r[i] = aR + (bR - aR) * f;

            fracPos += ratio;
            while (fracPos >= 1.0)
            {
                fracPos -= 1.0;
                readPos = (readPos + 1) % capacity;
                if (used > 0) --used;
            }
        }
        updateServo();
        return ok;
    }

    int getFill() const noexcept { return used; }
    double getRatio() const noexcept { return ratio; }

private:
    void updateServo() noexcept
    {
        // proportional rate servo: if the FIFO runs full/empty, nudge ratio by
        // at most +/-0.4 % so pitch stays imperceptible
        const double err = ((double) used - (double) targetFill) / (double) targetFill;   // -1..1
        const double nominal = srcRate / dstRate;
        ratio = nominal * (1.0 + clampd (err * 0.004, -0.004, 0.004));
    }

    std::vector<float> bufL, bufR;
    int capacity = 8192, readPos = 0, writePos = 0, used = 0;
    int targetFill = 4096;
    double srcRate = 48000.0, dstRate = 48000.0, ratio = 1.0, fracPos = 0.0;
};

// ----------------------------------------------------------------------------
//  Ring buffer used to hand spectrum data to the UI thread safely
// ----------------------------------------------------------------------------
class MonoPeakMeter
{
public:
    void prepare (double sr) noexcept { sampleRate = sr; reset(); }
    void reset() noexcept { peak = 0.0f; rms = 0.0f; }

    void push (const float* s, int n) noexcept
    {
        float pk = 0.0f, sum = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (s[i]);
            pk = std::max (pk, a);
            sum += s[i] * s[i];
        }
        peak = std::max (peak * decayPerBlock (n), pk);
        const float r = n > 0 ? std::sqrt (sum / (float) n) : 0.0f;
        rms = rms * 0.92f + r * 0.08f;
    }

    float getPeakDb() const noexcept { return gainToDecibels (peak.load()); }
    float getRmsDb()  const noexcept { return gainToDecibels (rms.load()); }

private:
    float decayPerBlock (int n) const noexcept
    {
        const float sec = (float) n / (float) (sampleRate > 0 ? sampleRate : 48000.0);
        return std::exp (-sec / 0.35f);      // ~350 ms peak decay
    }

    std::atomic<float> peak { 0.0f }, rms { 0.0f };
    double sampleRate = 48000.0;
};

} // namespace audiofx
