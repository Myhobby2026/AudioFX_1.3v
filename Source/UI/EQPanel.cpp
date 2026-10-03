// ============================================================================
//  EQPanel.cpp
// ============================================================================
#include "EQPanel.h"
#include "State/Params.h"
#include "Theme.h"

EQPanel::EQPanel (audiofx::ProcessingChain& chainToUse)
    : ModulePanel ("10-BAND EQUALIZER", "ISO OCTAVES"), chain (chainToUse)
{
    static const char* names[10] = { "31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };

    for (int i = 0; i < 10; ++i)
    {
        bands[i] = std::make_unique<Knob> (names[i],
            [] (double v) { return juce::String (v, 1) + " dB"; },
            i < 3 ? theme::accent3 : (i > 6 ? theme::warn : theme::accent));
        addAndMakeVisible (*bands[i]);
        bands[i]->setRange ({ -audiofx::TenBandEQ::kRangeDb, audiofx::TenBandEQ::kRangeDb }, 0.1);
        bands[i]->setValue (0.0, juce::dontSendNotification);
        bands[i]->getSlider().setDoubleClickReturnValue (true, 0.0);
        const int band = i;
        bands[i]->setOnValueChange ([this, band] { pushToChain (band); });
    }

    addAndMakeVisible (flatButton);
    flatButton.onClick = [this] { flat(); };
}

void EQPanel::pushToChain (int band)
{
    chain.getEq().setGainDb (band, (float) bands[band]->getValue());
    if (onParamChanged)
        onParamChanged();
    repaint();
}

void EQPanel::flat()
{
    for (int i = 0; i < 10; ++i)
    {
        bands[i]->setValue (0.0, juce::sendNotificationSync);
    }
}

void EQPanel::syncFrom (const Params& p)
{
    for (int i = 0; i < 10; ++i)
        bands[i]->setValue (p.eqGainDb[i], juce::dontSendNotification);
    setBypassActive (p.eqBypass);
    repaint();
}

void EQPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);

    // live response curve behind the knobs
    auto plot = getLocalBounds().reduced (24, 46).toFloat();

    juce::Path curve;
    bool first = true;
    for (int i = 0; i <= 140; ++i)
    {
        const double t = (double) i / 140.0;
        const double freq = 25.0 * std::pow (800.0, t);          // 25 Hz .. 20 kHz
        const double magDb = audiofx::gainToDecibels ((float) chain.getEq().responseAt (freq));
        const float x = plot.getX() + (float) t * plot.getWidth();
        const float y = plot.getCentreY() - juce::jlimit (-18.0f, 18.0f, (float) magDb) * (plot.getHeight() * 0.5f / 18.0f);
        if (first) { curve.startNewSubPath (x, y); first = false; }
        else       curve.lineTo (x, y);
    }

    // 0 dB reference
    g.setColour (theme::panelEdge);
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

    g.setColour (theme::accent.withAlpha (0.12f));
    g.strokePath (curve, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
    g.setColour (theme::accent.withAlpha (0.85f));
    g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));

    g.setColour (theme::textFaint);
    g.setFont (theme::monoFont (9.5f));
    g.drawText ("dB", plot.getTopLeft().x - 2.0f, plot.getTopLeft().y - 2.0f, 30, 12,
                juce::Justification::centredLeft);
}

void EQPanel::resized()
{
    ModulePanel::resized();

    auto hdr = getLocalBounds().reduced (12, 8).removeFromTop (24);
    hdr.removeFromRight (58);                       // skip the ON/OFF LED
    flatButton.setBounds (hdr.removeFromRight (58));

    auto area = getLocalBounds().reduced (16, 40);

    // 10 knobs across
    const int n = 10;
    const int gap = 4;
    const int w = (area.getWidth() - gap * (n - 1)) / n;
    auto row = area;
    for (int i = 0; i < n; ++i)
    {
        bands[i]->setBounds (row.removeFromLeft (w));
        row.removeFromLeft (gap);
    }
}
