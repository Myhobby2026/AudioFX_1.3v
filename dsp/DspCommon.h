// ============================================================================
//  AudioFX - System-Wide Audio Effects Processor
//  DspCommon.h - shared primitives for the JUCE-free DSP core
// ============================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace audiofx
{

static constexpr double kPi = 3.14159265358979323846;

// ---- dB / gain helpers -----------------------------------------------------
inline float decibelsToGain (float dB) noexcept    { return std::pow (10.0f, dB * 0.05f); }
inline float gainToDecibels (float g) noexcept     { return g <= 1.0e-8f ? -160.0f : 20.0f * std::log10 (g); }
inline double decibelsToGain (double dB) noexcept  { return std::pow (10.0, dB * 0.05); }
inline double gainToDecibels (double g) noexcept   { return g <= 1.0e-12 ? -320.0 : 20.0 * std::log10 (g); }

inline float clampf (float v, float lo, float hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }
inline double clampd (double v, double lo, double hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

// One-pole smoother, useful for parameter smoothing (time constant in seconds)
class Smoothed
{
public:
    void setSampleRate (double sr) noexcept          { sampleRate = sr > 0 ? sr : 48000.0; setTimeConstant (timeConstantSec); }
    void setTimeConstant (double seconds) noexcept   { timeConstantSec = seconds; coeff = 1.0 - std::exp (-1.0 / (timeConstantSec * sampleRate)); }
    void snap (double v) noexcept                    { current = target = v; }
    void setTarget (double v) noexcept               { target = v; }
    double getTarget() const noexcept                { return target; }
    double getCurrent() const noexcept               { return current; }
    double next() noexcept                           { current += (target - current) * coeff; return current; }
    void skip (int n) noexcept { for (int i = 0; i < n; ++i) next(); }

private:
    double sampleRate = 48000.0, timeConstantSec = 0.03, coeff = 0.01, current = 0.0, target = 0.0;
};

// ---- RBJ cookbook biquad (transposed direct form II) ------------------------
class Biquad
{
public:
    void reset() noexcept { z1 = z2 = 0.0; }

    void setIdentity() noexcept { b0 = 1.0; b1 = b2 = a1 = a2 = 0.0; }

    void setPeaking (double fs, double f0, double gainDb, double Q) noexcept
    {
        const double A  = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * clampd (f0, 10.0, fs * 0.49) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * std::max (0.05, Q));

        set (1.0 + alpha * A, -2.0 * cw, 1.0 - alpha * A,
             1.0 + alpha / A, -2.0 * cw, 1.0 - alpha / A);
    }

    void setLowShelf (double fs, double f0, double gainDb, double Q = 0.70710678) noexcept
    {
        const double A  = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * clampd (f0, 10.0, fs * 0.49) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * Q);
        const double sq = 2.0 * std::sqrt (A) * alpha;

        set (A * ((A + 1.0) - (A - 1.0) * cw + sq),
             2.0 * A * ((A - 1.0) - (A + 1.0) * cw),
             A * ((A + 1.0) - (A - 1.0) * cw - sq),
                 (A + 1.0) + (A - 1.0) * cw + sq,
            -2.0 * ((A - 1.0) + (A + 1.0) * cw),
                 (A + 1.0) + (A - 1.0) * cw - sq);
    }

    void setHighShelf (double fs, double f0, double gainDb, double Q = 0.70710678) noexcept
    {
        const double A  = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * clampd (f0, 10.0, fs * 0.49) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * Q);
        const double sq = 2.0 * std::sqrt (A) * alpha;

        set (A * ((A + 1.0) + (A - 1.0) * cw + sq),
            -2.0 * A * ((A - 1.0) + (A + 1.0) * cw),
             A * ((A + 1.0) + (A - 1.0) * cw - sq),
                 (A + 1.0) - (A - 1.0) * cw + sq,
             2.0 * ((A - 1.0) - (A + 1.0) * cw),
                 (A + 1.0) - (A - 1.0) * cw - sq);
    }

    // 2nd-order Butterworth lowpass / highpass (used by FieldSurround crossover)
    void setLowpass (double fs, double f0) noexcept
    {
        const double w0 = 2.0 * kPi * clampd (f0, 10.0, fs * 0.49) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * 0.70710678);
        set ((1.0 - cw) * 0.5, 1.0 - cw, (1.0 - cw) * 0.5,
             1.0 + alpha, -2.0 * cw, 1.0 - alpha);
    }

    void setHighpass (double fs, double f0) noexcept
    {
        const double w0 = 2.0 * kPi * clampd (f0, 10.0, fs * 0.49) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * 0.70710678);
        set ((1.0 + cw) * 0.5, -(1.0 + cw), (1.0 + cw) * 0.5,
             1.0 + alpha, -2.0 * cw, 1.0 - alpha);
    }

    void setAllpass (double fs, double f0, double Q) noexcept
    {
        const double w0 = 2.0 * kPi * clampd (f0, 10.0, fs * 0.49) / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * std::max (0.05, Q));
        set (1.0 - alpha, -2.0 * cw, 1.0 + alpha,
             1.0 + alpha, -2.0 * cw, 1.0 - alpha);
    }

    float process (float x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }

    // Magnitude response at frequency f (for drawing the EQ curve)
    double magnitudeAt (double fs, double f) const noexcept
    {
        const double w  = 2.0 * kPi * f / fs;
        const double cw = std::cos (w), cw2 = std::cos (2.0 * w);
        const double sw = std::sin (w), sw2 = std::sin (2.0 * w);
        const double numRe = b0 + b1 * cw + b2 * cw2, numIm = -(b1 * sw + b2 * sw2);
        const double denRe = 1.0 + a1 * cw + a2 * cw2, denIm = -(a1 * sw + a2 * sw2);
        const double num = numRe * numRe + numIm * numIm;
        const double den = denRe * denRe + denIm * denIm;
        return den > 0 ? std::sqrt (num / den) : 0.0;
    }

private:
    void set (double B0, double B1, double B2, double A0, double A1, double A2) noexcept
    {
        const double inv = A0 != 0.0 ? 1.0 / A0 : 1.0;
        b0 = B0 * inv; b1 = B1 * inv; b2 = B2 * inv;
        a1 = A1 * inv; a2 = A2 * inv;
    }

    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;
};

// ---- Simple stereo delay line (fractional read, linear interpolation) ------
class DelayLine
{
public:
    void resize (int maxSamples)
    {
        size = std::max (4, maxSamples + 4);
        buf.assign ((size_t) size, 0.0f);
        w = 0;
    }

    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }

    void write (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) % size; }

    // delay in samples, may be fractional; must be < size - 2
    float read (double delaySamples) const noexcept
    {
        const double d = clampd (delaySamples, 1.0, size - 2.0);
        const double rp = (double) w - d;
        const int i0 = (int) std::floor (rp);
        const double frac = rp - (double) i0;
        const int m = size;
        const float a = buf[(size_t) ((i0 % m + m) % m)];
        const float b = buf[(size_t) (((i0 + 1) % m + m) % m)];
        return a + (float) frac * (b - a);
    }

private:
    std::vector<float> buf;
    int size = 4, w = 0;
};

} // namespace audiofx
