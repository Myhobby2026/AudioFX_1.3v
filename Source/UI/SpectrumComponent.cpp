// ============================================================================
//  SpectrumComponent.cpp
// ============================================================================
#include "SpectrumComponent.h"
#include "Theme.h"

SpectrumComponent::SpectrumComponent (audiofx::ProcessingChain& chainToUse)
    : chain (chainToUse)
{
    displayDb.assign (audiofx::SpectrumFft::kNumBins, -120.0f);
    displayHold.assign (audiofx::SpectrumFft::kNumBins, -120.0f);
    setOpaque (false);
    startTimerHz (30);
}

SpectrumComponent::~SpectrumComponent()
{
    stopTimer();
}

void SpectrumComponent::timerCallback()
{
    const auto& mags = chain.getSpectrum().getMagnitudesDb();
    const auto& hold = chain.getSpectrum().getPeakHoldDb();
    if ((int) mags.size() != audiofx::SpectrumFft::kNumBins)
        return;

    displayDb = mags;
    displayHold = hold;
    repaint();
}

float SpectrumComponent::xForFrequency (double hz) const
{
    const double t = std::log (hz / kMinHz) / std::log (kMaxHz / kMinHz);
    return (float) (getLocalBounds().toFloat().reduced (4.0f).getX()
                    + t * (getLocalBounds().toFloat().reduced (4.0f).getWidth() - 8.0f));
}

float SpectrumComponent::yForDb (float db) const
{
    const float t = juce::jlimit (0.0f, 1.0f, (db - kMinDb) / (kMaxDb - kMinDb));
    const auto b = getLocalBounds().toFloat().reduced (4.0f);
    return b.getBottom() - t * (b.getHeight() - 10.0f);
}

void SpectrumComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    theme::paintGlass (g, area, "SPECTRUM", "FFT 4096");

    auto plot = area.reduced (18, 32);
    auto pf = plot.toFloat();

    // ---- grid -----------------------------------------------------------------
    g.setFont (theme::monoFont (9.5f));
    static const double freqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    static const char*  labels[] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };

    for (size_t i = 0; i < 8; ++i)
    {
        const float x = xForFrequency (freqs[i]);
        g.setColour (theme::panelEdge.withAlpha (0.5f));
        g.drawVerticalLine ((int) x, pf.getY(), pf.getBottom());
        g.setColour (theme::textFaint);
        g.drawText (labels[i], (int) x - 20, (int) pf.getBottom() + 2, 40, 12,
                    juce::Justification::centred);
    }

    for (float db = -60.0f; db <= 0.0f; db += 12.0f)
    {
        const float y = yForDb (db);
        g.setColour (theme::panelEdge.withAlpha (0.35f));
        g.drawHorizontalLine ((int) y, pf.getX(), pf.getRight());
        g.setColour (theme::textFaint);
        g.drawText (juce::String ((int) db), (int) pf.getRight() + 2, (int) y - 6, 26, 12,
                    juce::Justification::centredLeft);
    }

    // ---- spectrum path --------------------------------------------------------
    spectrumPath.clear();
    holdPath.clear();
    bool first = true, holdFirst = true;

    for (int b = 2; b < audiofx::SpectrumFft::kNumBins; ++b)
    {
        const double freq = chain.getSpectrum().binToFrequency (b);
        if (freq < kMinHz || freq > kMaxHz)
            continue;

        const float x = xForFrequency (freq);
        const float y = yForDb (displayDb[(size_t) b]);
        const float yh = yForDb (displayHold[(size_t) b]);

        if (first) { spectrumPath.startNewSubPath (x, y); first = false; }
        else       spectrumPath.lineTo (x, y);

        if (holdFirst) { holdPath.startNewSubPath (x, yh); holdFirst = false; }
        else           holdPath.lineTo (x, yh);
    }

    if (! first)
    {
        auto fill = spectrumPath;
        fill.lineTo (pf.getRight(), pf.getBottom());
        fill.lineTo (pf.getX(), pf.getBottom());
        fill.closeSubPath();

        juce::ColourGradient grad (theme::accent.withAlpha (0.40f), 0.0f, pf.getY(),
                                   theme::accent.withAlpha (0.02f), 0.0f, pf.getBottom(), false);
        grad.addColour (0.55, juce::Colour (0xff2E86FF).withAlpha (0.22f));
        g.setGradientFill (grad);
        g.fillPath (fill);

        g.setColour (theme::accent.withAlpha (0.18f));
        g.strokePath (spectrumPath, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        g.setColour (theme::accent);
        g.strokePath (spectrumPath, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
    }

    if (! holdFirst)
    {
        g.setColour (theme::textBright.withAlpha (0.35f));
        g.strokePath (holdPath, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }
}
