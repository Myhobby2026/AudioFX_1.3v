// ============================================================================
//  ModulePanels.cpp
// ============================================================================
#include "ModulePanels.h"
#include "State/Params.h"
#include "Theme.h"

// ============================================================== reverb ======
ReverbPanel::ReverbPanel (audiofx::ProcessingChain& chainToUse)
    : ModulePanel ("REVERB", "SCHROEDER"), chain (chainToUse)
{
    addAndMakeVisible (room);  addAndMakeVisible (damp);
    addAndMakeVisible (wet);   addAndMakeVisible (width);

    room.setRange ({ 0.0, 1.0 }, 0.001);
    damp.setRange ({ 0.0, 1.0 }, 0.001);
    wet.setRange  ({ 0.0, 1.0 }, 0.001);
    width.setRange({ 0.0, 1.0 }, 0.001);

    room.setOnValueChange ([this] { chain.getReverb().setRoomSize ((float) room.getValue());  if (onParamChanged) onParamChanged(); });
    damp.setOnValueChange ([this] { chain.getReverb().setDamping ((float) damp.getValue());   if (onParamChanged) onParamChanged(); });
    wet.setOnValueChange  ([this] { chain.getReverb().setWetLevel ((float) wet.getValue());   if (onParamChanged) onParamChanged(); });
    width.setOnValueChange([this] { chain.getReverb().setWidth ((float) width.getValue());    if (onParamChanged) onParamChanged(); });

    addAndMakeVisible (freeze);
    freeze.onClick = [this]
    {
        chain.getReverb().setFreeze (freeze.getToggleState());
        if (onParamChanged) onParamChanged();
    };
}

void ReverbPanel::resized()
{
    ModulePanel::resized();
    auto area = getLocalBounds().reduced (14, 40);

    // FREEZE lives in a slim right column so knobs keep full height
    auto right = area.removeFromRight (86);
    freeze.setBounds (right.reduced (4, right.getHeight() / 2 - 17));

    // same knob metrics as COMP / LIMITER tabs: centred, capped width, full height
    const int w = juce::jlimit (90, 180, area.getWidth() / 4);
    auto knobs = area.withSizeKeepingCentre (w * 4, area.getHeight());
    room.setBounds  (knobs.removeFromLeft (w));
    damp.setBounds  (knobs.removeFromLeft (w));
    wet.setBounds   (knobs.removeFromLeft (w));
    width.setBounds (knobs.removeFromLeft (w));
}

void ReverbPanel::syncFrom (const Params& p)
{
    room.setValue (p.revRoom);  damp.setValue (p.revDamp);
    wet.setValue (p.revWet);    width.setValue (p.revWidth);
    chain.getReverb().setDryLevel (p.revDry);
    freeze.setToggleState (p.revFreeze, juce::dontSendNotification);
    setBypassActive (p.revBypass);
}

// ============================================================ surround ======
SurroundPanel::SurroundPanel (audiofx::ProcessingChain& chainToUse)
    : ModulePanel ("FIELD SURROUND", "VIRTUAL"), chain (chainToUse)
{
    addAndMakeVisible (field);
    addAndMakeVisible (amount); addAndMakeVisible (spread);
    addAndMakeVisible (bass);   addAndMakeVisible (center);

    amount.setRange ({ 0.0, 1.0 }, 0.001);
    spread.setRange ({ 0.5, 2.0 }, 0.001);
    bass.setRange  ({ 0.0, 1.0 }, 0.001);
    center.setRange({ 0.0, 1.5 }, 0.001);

    amount.setOnValueChange ([this] { chain.getSurround().setAmount ((float) amount.getValue());    updateViz(); });
    spread.setOnValueChange ([this] { chain.getSurround().setSpread ((float) spread.getValue());    updateViz(); });
    bass.setOnValueChange  ([this] { chain.getSurround().setBassFocus ((float) bass.getValue());    updateViz(); });
    center.setOnValueChange([this] { chain.getSurround().setCenter ((float) center.getValue());     updateViz(); });
}

void SurroundPanel::resized()
{
    ModulePanel::resized();
    auto area = getLocalBounds().reduced (14, 40);

    // visualiser moves to the right (like the meters) so knobs get full height
    auto right = area.removeFromRight (juce::jlimit (150, 240, area.getWidth() / 4));
    field.setBounds (right);

    // same knob metrics as COMP / LIMITER tabs
    const int w = juce::jlimit (90, 180, area.getWidth() / 4);
    auto knobs = area.withSizeKeepingCentre (w * 4, area.getHeight());
    amount.setBounds (knobs.removeFromLeft (w));
    spread.setBounds (knobs.removeFromLeft (w));
    bass.setBounds   (knobs.removeFromLeft (w));
    center.setBounds (knobs.removeFromLeft (w));
}

void SurroundPanel::syncFrom (const Params& p)
{
    amount.setValue (p.surAmount); spread.setValue (p.surSpread);
    bass.setValue (p.surBass);     center.setValue (p.surCenter);
    updateViz();
    setBypassActive (p.surBypass);
}

// ========================================================= compressor ======
CompressorPanel::CompressorPanel (audiofx::ProcessingChain& chainToUse)
    : ModulePanel ("COMPRESSOR", "SOFT KNEE"), chain (chainToUse)
{
    addAndMakeVisible (thresh); addAndMakeVisible (ratio);
    addAndMakeVisible (attack); addAndMakeVisible (release);
    addAndMakeVisible (makeup);
    addAndMakeVisible (grMeter); addAndMakeVisible (grLabel);

    thresh.setRange  ({ -40.0, 0.0 }, 0.1);
    ratio.setRange   ({ 1.0, 20.0 }, 0.1);
    attack.setRange  ({ 0.5, 200.0 }, 0.5);
    release.setRange ({ 10.0, 1000.0 }, 1.0);
    makeup.setRange  ({ 0.0, 24.0 }, 0.1);

    thresh.setOnValueChange  ([this] { chain.getCompressor().setThresholdDb ((float) thresh.getValue());  if (onParamChanged) onParamChanged(); });
    ratio.setOnValueChange   ([this] { chain.getCompressor().setRatio ((float) ratio.getValue());         if (onParamChanged) onParamChanged(); });
    attack.setOnValueChange  ([this] { chain.getCompressor().setAttackMs ((float) attack.getValue());     if (onParamChanged) onParamChanged(); });
    release.setOnValueChange ([this] { chain.getCompressor().setReleaseMs ((float) release.getValue());   if (onParamChanged) onParamChanged(); });
    makeup.setOnValueChange  ([this] { chain.getCompressor().setMakeupDb ((float) makeup.getValue());     if (onParamChanged) onParamChanged(); });

    grMeter.setAccent (theme::accent2);
    grMeter.setRangeDb (0.0f, 24.0f);
    grLabel.setFont (theme::font (10.5f));
    grLabel.setColour (juce::Label::textColourId, theme::textDim);
    grLabel.setJustificationType (juce::Justification::centred);
}

void CompressorPanel::resized()
{
    ModulePanel::resized();
    auto area = getLocalBounds().reduced (14, 40);

    auto right = area.removeFromRight (30);
    grMeter.setBounds (right.removeFromTop (right.getHeight() - 16));
    grLabel.setBounds (right);

    const int w = area.getWidth() / 5;
    thresh.setBounds  (area.removeFromLeft (w));
    ratio.setBounds   (area.removeFromLeft (w));
    attack.setBounds  (area.removeFromLeft (w));
    release.setBounds (area.removeFromLeft (w));
    makeup.setBounds  (area);
}

void CompressorPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    auto tag = getLocalBounds().reduced (12, 8).removeFromTop (24);
    g.setColour (theme::textFaint);
    g.setFont (theme::monoFont (10.5f));
    g.drawText (juce::String (chain.getCompressor().getGainReductionDb(), 1) + " dB GR",
                tag.removeFromLeft (100), juce::Justification::centredLeft, false);
}

void CompressorPanel::refreshMeters()
{
    grMeter.setReductionDb (chain.getCompressor().getGainReductionDb());
}

void CompressorPanel::syncFrom (const Params& p)
{
    thresh.setValue (p.compThresholdDb);
    ratio.setValue (p.compRatio);
    attack.setValue (p.compAttackMs);
    release.setValue (p.compReleaseMs);
    makeup.setValue (p.compMakeupDb);
    setBypassActive (p.compBypass);
}

// ============================================================= limiter ======
LimiterPanel::LimiterPanel (audiofx::ProcessingChain& chainToUse)
    : ModulePanel ("LIMITER", "LOOKAHEAD"), chain (chainToUse)
{
    addAndMakeVisible (ceiling); addAndMakeVisible (release);
    addAndMakeVisible (grMeter); addAndMakeVisible (grLabel);

    ceiling.setRange ({ -24.0, 0.0 }, 0.1);
    release.setRange ({ 5.0, 1000.0 }, 1.0);

    ceiling.setOnValueChange ([this] { chain.getLimiter().setCeilingDb ((float) ceiling.getValue()); if (onParamChanged) onParamChanged(); });
    release.setOnValueChange ([this] { chain.getLimiter().setReleaseMs ((float) release.getValue()); if (onParamChanged) onParamChanged(); });

    grMeter.setAccent (theme::accent2);
    grMeter.setRangeDb (0.0f, 18.0f);
    grLabel.setFont (theme::font (10.5f));
    grLabel.setColour (juce::Label::textColourId, theme::textDim);
    grLabel.setJustificationType (juce::Justification::centred);
}

void LimiterPanel::resized()
{
    ModulePanel::resized();
    auto area = getLocalBounds().reduced (14, 40);

    auto right = area.removeFromRight (30);
    grMeter.setBounds (right.removeFromTop (right.getHeight() - 16));
    grLabel.setBounds (right);

    const int w = area.getWidth() / 2;
    ceiling.setBounds (area.removeFromLeft (w));
    release.setBounds (area);
}

void LimiterPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    auto tag = getLocalBounds().reduced (12, 8).removeFromTop (24);
    g.setColour (theme::textFaint);
    g.setFont (theme::monoFont (10.5f));
    g.drawText (juce::String (chain.getLimiter().getGainReductionDb(), 1) + " dB GR",
                tag.removeFromLeft (100), juce::Justification::centredLeft, false);
}

void LimiterPanel::refreshMeters()
{
    grMeter.setReductionDb (chain.getLimiter().getGainReductionDb());
}

void LimiterPanel::syncFrom (const Params& p)
{
    ceiling.setValue (p.limCeilingDb);
    release.setValue (p.limReleaseMs);
    setBypassActive (p.limBypass);
}

// =============================================================== master =====
MasterPanel::MasterPanel (audiofx::ProcessingChain& chainToUse)
    : ModulePanel ("MASTER", "AGC"), chain (chainToUse)
{
    addAndMakeVisible (volume); addAndMakeVisible (agcTarget);
    addAndMakeVisible (agc);
    addAndMakeVisible (meterL); addAndMakeVisible (meterR);
    addAndMakeVisible (levelReadout);

    volume.setRange ({ -60.0, 12.0 }, 0.1);
    agcTarget.setRange ({ -36.0, -6.0 }, 0.5);
    volume.getSlider().setDoubleClickReturnValue (true, -6.0);

    volume.setOnValueChange ([this] { chain.getMaster().setVolumeDb ((float) volume.getValue()); if (onParamChanged) onParamChanged(); });
    agcTarget.setOnValueChange ([this] { chain.getMaster().setAgcTargetDb ((float) agcTarget.getValue()); if (onParamChanged) onParamChanged(); });

    agc.setClickingTogglesState (true);
    agc.setToggleState (true, juce::dontSendNotification);
    agc.onClick = [this]
    {
        chain.getMaster().setAgcEnabled (agc.getToggleState());
        if (onParamChanged) onParamChanged();
    };

    levelReadout.setFont (theme::monoFont (12.0f, true));
    levelReadout.setColour (juce::Label::textColourId, theme::accent3);
    levelReadout.setJustificationType (juce::Justification::centred);
    levelReadout.setText ("-inf dBFS", juce::dontSendNotification);
}

void MasterPanel::resized()
{
    ModulePanel::resized();
    auto area = getLocalBounds().reduced (14, 38);

    levelReadout.setBounds (area.removeFromBottom (22));

    auto meters = area.removeFromRight (34);
    const int barW = 14;
    meterL.setBounds (meters.removeFromLeft (barW));
    meters.removeFromLeft (4);
    meterR.setBounds (meters.removeFromLeft (barW));

    auto knobs = area;
    auto top = knobs.removeFromTop (knobs.getHeight() / 2 + 10);
    volume.setBounds (top);
    agcTarget.setBounds (knobs.removeFromLeft (knobs.getWidth() * 3 / 5));
    agc.setBounds (knobs.reduced (4, 18));
}

void MasterPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);

    // big value readout above the volume knob
    auto tag = getLocalBounds().reduced (12, 8).removeFromTop (24);
    g.setColour (theme::textDim);
    g.setFont (theme::monoFont (11.0f));
    const float vol = chain.getMaster().getVolumeDb();
    g.drawText (juce::String (vol, 1) + " dB", tag.removeFromLeft (100),
                juce::Justification::centredLeft, false);
}

void MasterPanel::refreshMeters()
{
    const auto& out = chain.getOutMeter();
    meterL.setLevelDb (out.getPeakDb(), out.getRmsDb());
    meterR.setLevelDb (out.getPeakDb(), out.getRmsDb());

    const float prog = chain.getMaster().getProgrammeDb();
    const float agcG = chain.getMaster().getAgcGainDb();
    levelReadout.setText (juce::String (prog, 1) + " dBFS   AGC " + juce::String (agcG, 1) + " dB",
                          juce::dontSendNotification);
}

void MasterPanel::syncFrom (const Params& p)
{
    volume.setValue (p.volDb);
    agcTarget.setValue (p.agcTargetDb);
    agc.setToggleState (p.agcEnabled, juce::dontSendNotification);
}
