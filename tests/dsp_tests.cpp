// ============================================================================
//  dsp_tests.cpp - numeric validation of the AudioFX DSP core (no JUCE needed)
//
//  Build:  g++ -std=c++17 -O2 dsp_tests.cpp -o dsp_tests && ./dsp_tests
//  NOTE:   process() consumes from the *current* pointer position - tests must
//          advance the channel pointers each block (float* chs[] = {buf+off}).
// ============================================================================
#include "../dsp/DspCommon.h"
#include "../dsp/TenBandEQ.h"
#include "../dsp/ReverbEngine.h"
#include "../dsp/FieldSurround.h"
#include "../dsp/LookaheadLimiter.h"
#include "../dsp/MasterBus.h"
#include "../dsp/SpectrumFft.h"
#include "../dsp/AudioFifo.h"
#include "../dsp/ProcessingChain.h"

#include <cstdio>
#include <cstring>
#include <random>

using namespace audiofx;

static int gFailures = 0;

#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        if (cond) { std::printf ("  [PASS] %s\n", msg); }                       \
        else      { std::printf ("  [FAIL] %s\n", msg); ++gFailures; }          \
    } while (0)

static bool isFinite (const float* d, int n)
{
    for (int i = 0; i < n; ++i)
        if (! std::isfinite (d[i]))
            return false;
    return true;
}

static float rmsOf (const float* d, int n)
{
    double s = 0;
    for (int i = 0; i < n; ++i) s += (double) d[i] * d[i];
    return (float) std::sqrt (s / std::max (1, n));
}

static float peakOf (const float* d, int n)
{
    float p = 0;
    for (int i = 0; i < n; ++i) p = std::max (p, std::abs (d[i]));
    return p;
}

static void makeSine (float* d, int n, double fs, double freq, float amp = 0.5f)
{
    for (int i = 0; i < n; ++i)
        d[i] = amp * (float) std::sin (2.0 * kPi * freq * (double) i / fs);
}

// Run a stereo (or mono) buffer through a processor block by block,
// advancing pointers exactly like a real audio driver would.
template <typename Proc>
static void runBlocks (Proc& p, float* l, float* r, int totalSamples, int block = 512)
{
    for (int off = 0; off + block <= totalSamples; off += block)
    {
        float* chs[2] = { l + off, r != nullptr ? r + off : nullptr };
        p.process (chs, r != nullptr ? 2 : 1, block);
    }
}

// ----------------------------------------------------------------------------
static void testEQ()
{
    std::printf ("\n== TenBandEQ ==\n");
    const double fs = 48000.0;
    const int N = 48000;
    std::vector<float> buf ((size_t) N), ref ((size_t) N);

    TenBandEQ eq;
    eq.prepare (fs, 512);

    makeSine (buf.data(), N, fs, 1000.0, 0.25f);
    std::memcpy (ref.data(), buf.data(), sizeof (float) * (size_t) N);

    // flat
    runBlocks (eq, buf.data(), nullptr, N);
    float flatDb = gainToDecibels (rmsOf (buf.data() + 24000, N - 24000)
                                   / rmsOf (ref.data() + 24000, N - 24000));
    CHECK (std::abs (flatDb) < 0.5f, "flat response within 0.5 dB");

    // +12 dB at the 1 kHz band -> +12 dB at 1 kHz
    eq.reset();
    eq.setGainDb (5, 12.0f);
    std::memcpy (buf.data(), ref.data(), sizeof (float) * (size_t) N);
    runBlocks (eq, buf.data(), nullptr, N);
    float boostDb = gainToDecibels (rmsOf (buf.data() + 24000, N - 24000)
                                    / rmsOf (ref.data() + 24000, N - 24000));
    std::printf ("       (measured boost = %.2f dB)\n", boostDb);
    CHECK (boostDb > 11.0f && boostDb < 13.2f, "+12 dB band boost at 1 kHz measured");

    // -12 dB cut at 1 kHz
    eq.reset();
    eq.setGainDb (5, -12.0f);
    std::memcpy (buf.data(), ref.data(), sizeof (float) * (size_t) N);
    runBlocks (eq, buf.data(), nullptr, N);
    float cutDb = gainToDecibels (rmsOf (buf.data() + 24000, N - 24000)
                                  / rmsOf (ref.data() + 24000, N - 24000));
    CHECK (cutDb < -11.0f && cutDb > -13.2f, "-12 dB band cut at 1 kHz measured");

    // boost at 1 kHz must not move 100 Hz much
    eq.reset();
    eq.setGainDb (5, 12.0f);
    makeSine (buf.data(), N, fs, 100.0, 0.25f);
    runBlocks (eq, buf.data(), nullptr, N);
    float sideDb = gainToDecibels (rmsOf (buf.data() + 24000, N - 24000) / (0.25f / std::sqrt (2.0f)));
    CHECK (std::abs (sideDb) < 2.5f, "1 kHz boost stays out of 100 Hz (<2.5 dB)");

    CHECK (isFinite (buf.data(), N), "output finite (no NaN)");

    // extreme settings stress: all bands boosted hard
    TenBandEQ hot;
    hot.prepare (fs, 512);
    for (int b = 0; b < 10; ++b) hot.setGainDb (b, 15.0f);
    std::vector<float> noise (48000);
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
    for (auto& s : noise) s = dist (rng);
    runBlocks (hot, noise.data(), nullptr, 48000);
    CHECK (isFinite (noise.data(), 48000), "all-bands +15 dB stress: finite");
}

// ----------------------------------------------------------------------------
static void testReverb()
{
    std::printf ("\n== ReverbEngine ==\n");
    const double fs = 48000.0;
    const int N = 48000 * 3;
    std::vector<float> L ((size_t) N, 0.0f), R ((size_t) N, 0.0f);

    ReverbEngine rev;
    rev.prepare (fs, 512);
    rev.setRoomSize (0.85f);
    rev.setDamping (0.25f);
    rev.setWetLevel (1.0f);
    rev.setDryLevel (0.0f);

    L[100] = 1.0f; R[100] = 1.0f;                     // impulse

    runBlocks (rev, L.data(), R.data(), N);

    CHECK (isFinite (L.data(), N) && isFinite (R.data(), N), "output finite (no NaN)");

    float tailEarly = std::max (peakOf (L.data() + 4800, 4800), peakOf (R.data() + 4800, 4800));
    float tailLate  = std::max (peakOf (L.data() + 72000, 4800), peakOf (R.data() + 72000, 4800));
    float tailPeak  = std::max (peakOf (L.data(), N), peakOf (R.data(), N));
    std::printf ("       (tail@100ms=%.5f tail@1.5s=%.5f peak=%.5f)\n", tailEarly, tailLate, tailPeak);
    CHECK (tailEarly > 1.0e-4f, "impulse produces audible tail at 100 ms");
    CHECK (tailLate < tailEarly, "tail decays over 1.5 s");
    CHECK (tailPeak < 5.0f, "wet tail stays bounded (peak < 5)");

    // dry path must pass through untouched
    ReverbEngine dry;
    dry.prepare (fs, 512);
    dry.setWetLevel (0.0f); dry.setDryLevel (1.0f);
    std::vector<float> dL (512), dR (512);
    for (int i = 0; i < 512; ++i) { dL[i] = 0.3f; dR[i] = -0.3f; }
    float* dch[2] = { dL.data(), dR.data() };
    dry.process (dch, 2, 512);
    CHECK (std::abs (dL[400] - 0.3f) < 1.0e-3f && std::abs (dR[400] + 0.3f) < 1.0e-3f, "dry signal passes clean");

    // sustained input bounded
    ReverbEngine sus;
    sus.prepare (fs, 512);
    sus.setRoomSize (0.9f); sus.setWetLevel (0.5f); sus.setDryLevel (1.0f);
    std::vector<float> sL (48000), sR (48000);
    makeSine (sL.data(), 48000, fs, 220.0, 0.5f);
    std::memcpy (sR.data(), sL.data(), sizeof (float) * 48000);
    runBlocks (sus, sL.data(), sR.data(), 48000);
    float sp = std::max (peakOf (sL.data(), 48000), peakOf (sR.data(), 48000));
    CHECK (sp < 3.0f && isFinite (sL.data(), 48000), "sustained 0.5 sine + wet 0.5 bounded");
}

// ----------------------------------------------------------------------------
static void testSurround()
{
    std::printf ("\n== FieldSurround ==\n");
    const double fs = 48000.0;
    const int N = 24000;
    std::vector<float> L ((size_t) N), R ((size_t) N);

    FieldSurround fsur;
    fsur.prepare (fs, 512);
    fsur.setAmount (0.8f);
    fsur.setSpread (1.6f);
    fsur.setBassFocus (0.8f);

    makeSine (L.data(), N, fs, 1000.0, 0.3f);
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);   // dual-mono

    runBlocks (fsur, L.data(), R.data(), N);

    CHECK (isFinite (L.data(), N) && isFinite (R.data(), N), "output finite (no NaN)");

    double diff = 0, tot = 0;
    for (int i = 4800; i < N; ++i)
    {
        diff += (double) (L[i] - R[i]) * (L[i] - R[i]);
        tot  += (double) L[i] * L[i] + (double) R[i] * R[i];
    }
    const double w = diff / std::max (tot, 1.0e-12);
    std::printf ("       (dual-mono width metric = %.4f)\n", w);
    CHECK (w > 0.01, "dual-mono decorrelates into a wide field");

    // bass focus: 50 Hz tone should stay nearly mono even at high spread
    fsur.reset();
    fsur.setAmount (1.0f); fsur.setSpread (2.0f); fsur.setBassFocus (1.0f);
    makeSine (L.data(), N, fs, 50.0, 0.3f);
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);
    runBlocks (fsur, L.data(), R.data(), N);
    diff = 0; tot = 0;
    for (int i = 4800; i < N; ++i)
    {
        diff += (double) (L[i] - R[i]) * (L[i] - R[i]);
        tot  += (double) L[i] * L[i] + (double) R[i] * R[i];
    }
    CHECK (diff / std::max (tot, 1.0e-12) < 0.02, "bass focus keeps 50 Hz mono-anchored");

    float pk = std::max (peakOf (L.data(), N), peakOf (R.data(), N));
    CHECK (pk < 1.5f, "no runaway gain (peak < 1.5 for 0.3 input)");

    // amount=0 must be a pure M/S width pass (mono in -> mono out)
    FieldSurround pure;
    pure.prepare (fs, 512);
    pure.setAmount (0.0f); pure.setSpread (1.5f); pure.setBassFocus (0.0f);
    makeSine (L.data(), N, fs, 700.0, 0.3f);
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);
    runBlocks (pure, L.data(), R.data(), N);
    diff = 0;
    for (int i = 4800; i < N; ++i)
        diff += (double) (L[i] - R[i]) * (L[i] - R[i]);
    CHECK (diff < 1.0e-6, "amount=0 leaves dual-mono untouched (no width)");
}

// ----------------------------------------------------------------------------
static void testLimiter()
{
    std::printf ("\n== LookaheadLimiter ==\n");
    const double fs = 48000.0;
    const int N = 512 * 93;                            // exact block coverage
    std::vector<float> L ((size_t) N), R ((size_t) N);

    LookaheadLimiter lim;
    lim.prepare (fs, 512);
    lim.setCeilingDb (-1.0f);
    lim.setReleaseMs (80.0f);

    makeSine (L.data(), N, fs, 997.0, 4.0f);          // 4.0 = +12 dBFS hot!
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);

    float midRunGr = 0.0f;
    for (int off = 0; off + 512 <= N; off += 512)
    {
        float* chs[2] = { L.data() + off, R.data() + off };
        lim.process (chs, 2, 512);
        if (off == 24064) midRunGr = lim.getGainReductionDb();
    }

    const float ceilingLin = decibelsToGain (-1.0f);
    float pk = std::max (peakOf (L.data(), N), peakOf (R.data(), N));
    std::printf ("       (peak=%.4f ceiling=%.4f mid-run GR=%.2f dB)\n", pk, ceilingLin, midRunGr);
    CHECK (isFinite (L.data(), N) && isFinite (R.data(), N), "output finite (no NaN)");
    CHECK (pk <= ceilingLin * 1.0005f, "hot +12 dBFS sine clamped to -1 dBFS ceiling");
    CHECK (midRunGr > 10.0f, "GR meter reports > 10 dB reduction mid-run");

    // quiet signal must pass with unity gain (limiter transparent)
    lim.reset();
    makeSine (L.data(), N, fs, 997.0, 0.1f);
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);
    runBlocks (lim, L.data(), R.data(), N);
    float quietRms = rmsOf (L.data() + 4800, N - 4800);
    CHECK (gainToDecibels (quietRms / (0.1f / std::sqrt (2.0f))) > -0.6f, "quiet sine passes at unity");

    // impulse train: spikes must never exceed the ceiling
    lim.reset();
    std::fill (L.begin(), L.end(), 0.0f);
    std::fill (R.begin(), R.end(), 0.0f);
    for (int i = 0; i < N; i += 3000)
        { L[i] = ((i / 3000) % 2 ? -1.0f : 1.0f) * 3.0f; R[i] = L[i]; }
    runBlocks (lim, L.data(), R.data(), N);
    pk = std::max (peakOf (L.data(), N), peakOf (R.data(), N));
    CHECK (pk <= ceilingLin * 1.0005f, "impulse spikes clamped to ceiling");
}

// ----------------------------------------------------------------------------
static void testMaster()
{
    std::printf ("\n== MasterBus / AGC ==\n");
    const double fs = 48000.0;
    const int N = 48000 * 12;
    std::vector<float> L ((size_t) N), R ((size_t) N);

    MasterBus mb;
    mb.prepare (fs, 512);
    mb.setVolumeDb (0.0f);
    mb.setAgcEnabled (true);
    mb.setAgcTargetDb (-18.0f);

    makeSine (L.data(), N, fs, 1000.0, 0.05f);        // quiet programme
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);

    runBlocks (mb, L.data(), R.data(), N);

    float outRms = rmsOf (L.data() + N - 48000, 48000);
    float outDb = gainToDecibels (outRms);
    std::printf ("       (final output level = %.2f dBFS, agc gain = %.2f dB)\n", outDb, mb.getAgcGainDb());
    CHECK (outDb > -21.0f && outDb < -15.0f, "AGC pulls programme to -18 dBFS +/-3 dB");

    // volume knob maths
    MasterBus mb2;
    mb2.prepare (fs, 512);
    mb2.setAgcEnabled (false);
    mb2.setVolumeDb (-6.0f);
    makeSine (L.data(), 48000, fs, 1000.0, 0.5f);
    runBlocks (mb2, L.data(), nullptr, 48000);
    float vDb = gainToDecibels (rmsOf (L.data() + 24000, 24000) / (0.5f / std::sqrt (2.0f)));
    std::printf ("       (volume knob = %.2f dB)\n", vDb);
    CHECK (vDb > -6.6f && vDb < -5.4f, "-6 dB volume knob lands within 0.6 dB");
}

// ----------------------------------------------------------------------------
static void testSpectrum()
{
    std::printf ("\n== SpectrumFft ==\n");
    const double fs = 48000.0;
    SpectrumFft sp;
    sp.prepare (fs);

    std::vector<float> buf (4096);
    for (int blk = 0; blk < 32; ++blk)
    {
        for (int i = 0; i < 4096; ++i)
        {
            const double t = (double) (blk * 4096 + i) / fs;
            buf[(size_t) i] = 0.5f * (float) std::sin (2.0 * kPi * 1000.0 * t);
        }
        sp.push (buf.data(), 4096);
    }

    const auto& mags = sp.getMagnitudesDb();
    int peakBin = 0;
    float peakDb = -200.0f;
    for (int b = 2; b < SpectrumFft::kNumBins; ++b)
        if (mags[(size_t) b] > peakDb) { peakDb = mags[(size_t) b]; peakBin = b; }

    const double peakHz = sp.binToFrequency (peakBin);
    std::printf ("       (peak bin %d = %.1f Hz, %.1f dB)\n", peakBin, peakHz, peakDb);
    CHECK (peakHz > 950.0 && peakHz < 1050.0, "1 kHz sine detected at 1 kHz (+/-1 bin)");
    CHECK (peakDb > -15.0f && peakDb < -8.0f, "sine level sane in dB display (Hann-scaled)");

    float other = -120.0f;
    for (int b = 2; b < SpectrumFft::kNumBins; ++b)
        if (std::abs (b - peakBin) > 4)
            other = std::max (other, mags[(size_t) b]);
    CHECK (peakDb - other > 20.0f, "tone stands >20 dB above surrounding bins");
}

// ----------------------------------------------------------------------------
static void testFifo()
{
    std::printf ("\n== AdaptiveStereoFifo ==\n");
    AdaptiveStereoFifo fifo;
    fifo.prepare (48000.0, 8192);
    fifo.setSourceRate (44100.0);                      // mismatched clocks
    fifo.setDestRate (48000.0);

    std::vector<float> inL (256), inR (256), outL (256), outR (256);
    makeSine (inL.data(), 256, 44100.0, 440.0, 0.5f);
    std::memcpy (inR.data(), inL.data(), sizeof (float) * 256);

    int underruns = 0;
    for (int iter = 0; iter < 2000; ++iter)
    {
        fifo.push (inL.data(), inR.data(), 256);
        if (! fifo.pull (outL.data(), outR.data(), 256))
            ++underruns;
    }
    CHECK (isFinite (outL.data(), 256), "resampled output finite");
    CHECK (fifo.getFill() > 0 && fifo.getFill() < 8192, "FIFO fill stays bounded under clock drift");
    CHECK (underruns < 50, "rare underruns while rate-servoing 44.1k -> 48k");
    CHECK (fifo.getRatio() > 0.9 && fifo.getRatio() < 0.95, "servo ratio near 44100/48000");
    std::printf ("       (fill=%d, ratio=%.5f, underruns=%d)\n", fifo.getFill(), fifo.getRatio(), underruns);
}

// ----------------------------------------------------------------------------
static void testChain()
{
    std::printf ("\n== ProcessingChain (smoke / stress) ==\n");
    const double fs = 48000.0;
    ProcessingChain chain;
    chain.prepare (fs, 512);

    for (int b = 0; b < 10; ++b)
        chain.getEq().setGainDb (b, (float) ((b % 3 - 1) * 8));
    chain.getReverb().setRoomSize (0.8f);
    chain.getReverb().setWetLevel (0.4f);
    chain.getSurround().setAmount (0.9f);
    chain.getSurround().setSpread (1.8f);
    chain.getMaster().setVolumeDb (0.0f);
    chain.getMaster().setAgcEnabled (true);
    chain.getLimiter().setCeilingDb (-1.0f);

    std::mt19937 rng (12345);
    std::uniform_real_distribution<float> dist (-3.0f, 3.0f);
    std::vector<float> L (512), R (512);
    float worst = 0.0f;
    bool finite = true;

    for (int blk = 0; blk < 2000; ++blk)
    {
        for (int i = 0; i < 512; ++i) { L[i] = dist (rng); R[i] = dist (rng); }
        float* chs[2] = { L.data(), R.data() };
        chain.process (chs, 2, 512);
        worst = std::max (worst, std::max (peakOf (L.data(), 512), peakOf (R.data(), 512)));
        finite = finite && isFinite (L.data(), 512) && isFinite (R.data(), 512);
    }

    CHECK (finite, "2000 blocks of noise through full chain: all finite");
    const float ceilLin = decibelsToGain (-1.0f);
    CHECK (worst <= ceilLin * 1.0005f, "chain output never exceeds -1 dBFS ceiling");
    std::printf ("       (worst peak = %.3f, ceiling = %.3f)\n", worst, ceilLin);

    // global bypass must be bit-transparent
    chain.setGlobalBypass (true);
    std::vector<float> orig (512);
    makeSine (L.data(), 512, fs, 440.0, 0.4f);
    std::memcpy (R.data(), L.data(), sizeof (float) * 512);
    std::memcpy (orig.data(), L.data(), sizeof (float) * 512);
    float* chs[2] = { L.data(), R.data() };
    chain.process (chs, 2, 512);
    bool same = true;
    for (int i = 0; i < 512; ++i)
        if (L[i] != orig[(size_t) i]) same = false;
    CHECK (same, "global bypass is bit-transparent");
}

// ----------------------------------------------------------------------------
static void testCompressor()
{
    std::printf ("\n== Compressor ==\n");
    const double fs = 48000.0;
    const int N = 48000 * 2;
    std::vector<float> L ((size_t) N), R ((size_t) N);

    Compressor comp;
    comp.prepare (fs, 256);
    comp.setThresholdDb (-20.0f);
    comp.setRatio (4.0f);
    comp.setAttackMs (5.0f);
    comp.setReleaseMs (100.0f);
    comp.setMakeupDb (0.0f);

    // hot sine: 0.5 peak = -6 dBFS, well above the -20 dB threshold
    makeSine (L.data(), N, fs, 997.0, 0.5f);
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);
    runBlocks (comp, L.data(), R.data(), N);

    const float inRms  = 0.5f / std::sqrt (2.0f);
    const float outRms = rmsOf (L.data() + 48000, N - 48000);   // last 1 s (steady GR)
    const float gr = gainToDecibels (inRms) - gainToDecibels (outRms);
    std::printf ("       (steady GR=%.2f dB, meter=%.2f dB)\n", gr, (double) comp.getGainReductionDb());
    CHECK (isFinite (L.data(), N) && isFinite (R.data(), N), "output finite (no NaN)");
    CHECK (gr > 7.0f && gr < 13.0f, "steady reduction ~10.5 dB (-6 dBFS in, thr -20, 4:1)");
    CHECK (comp.getGainReductionDb() > 7.0f && comp.getGainReductionDb() < 13.0f, "GR meter reads ~10.5 dB");

    // below-threshold signal passes at unity
    comp.reset();
    makeSine (L.data(), N, fs, 997.0, 0.02f);                  // -34 dBFS peak
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);
    runBlocks (comp, L.data(), R.data(), N);
    const float quietRms = rmsOf (L.data() + 4800, N - 4800);
    CHECK (std::abs (gainToDecibels (quietRms / (0.02f / std::sqrt (2.0f)))) < 0.8f,
           "below-threshold signal passes at unity");

    // makeup gain lifts the compressed signal by ~6 dB
    comp.reset();
    comp.setMakeupDb (6.0f);
    makeSine (L.data(), N, fs, 997.0, 0.5f);
    std::memcpy (R.data(), L.data(), sizeof (float) * (size_t) N);
    runBlocks (comp, L.data(), R.data(), N);
    const float makeupRms = rmsOf (L.data() + 48000, N - 48000);
    const float lift = gainToDecibels (makeupRms) - gainToDecibels (outRms);
    CHECK (lift > 5.0f && lift < 7.0f, "makeup +6 dB lifts output by ~6 dB");

    // bypass must be bit-transparent
    comp.reset();
    comp.setBypassed (true);
    makeSine (L.data(), 256, fs, 997.0, 0.5f);
    std::vector<float> orig (L.begin(), L.begin() + 256);
    float* chs[2] = { L.data(), R.data() };
    comp.process (chs, 2, 256);
    bool same = true;
    for (int i = 0; i < 256; ++i)
        if (L[i] != orig[(size_t) i]) same = false;
    CHECK (same, "bypass is bit-transparent");
}

// ----------------------------------------------------------------------------
int main()
{
    std::printf ("AudioFX DSP core validation\n");
    testEQ();
    testReverb();
    testSurround();
    testLimiter();
    testCompressor();
    testMaster();
    testSpectrum();
    testFifo();
    testChain();

    std::printf ("\n%s (%d failure%s)\n", gFailures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 gFailures, gFailures == 1 ? "" : "s");
    return gFailures == 0 ? 0 : 1;
}
