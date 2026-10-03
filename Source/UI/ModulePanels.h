// ============================================================================
//  ModulePanels.h - Reverb / Field Surround / Limiter / Master panels
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Controls.h"
#include "dsp/ProcessingChain.h"

class ReverbPanel : public ModulePanel
{
public:
    explicit ReverbPanel (audiofx::ProcessingChain& chain);
    void resized() override;
    void syncFrom (const struct Params& p);
    std::function<void()> onParamChanged;

private:
    audiofx::ProcessingChain& chain;
    Knob room { "ROOM",   [] (double v) { return juce::String ((int) std::round (v * 100)) + " %"; } };
    Knob damp { "DAMP",   [] (double v) { return juce::String ((int) std::round (v * 100)) + " %"; } };
    Knob wet  { "WET",    [] (double v) { return juce::String ((int) std::round (v * 100)) + " %"; }, theme::accent2 };
    Knob width{ "WIDTH",  [] (double v) { return juce::String ((int) std::round (v * 100)) + " %"; } };
    juce::ToggleButton freeze { "FREEZE" };
};

class SurroundPanel : public ModulePanel
{
public:
    explicit SurroundPanel (audiofx::ProcessingChain& chain);
    void resized() override;
    void syncFrom (const struct Params& p);
    std::function<void()> onParamChanged;

private:
    void updateViz()
    {
        field.setParams ((float) amount.getValue(), (float) spread.getValue(), (float) bass.getValue());
        if (onParamChanged)
            onParamChanged();
    }

    audiofx::ProcessingChain& chain;
    FieldVisualizer field;
    Knob amount { "FIELD",  [] (double v) { return juce::String ((int) std::round (v * 100)) + " %"; } };
    Knob spread { "SPREAD", [] (double v) { return juce::String (v, 2) + " x"; } };
    Knob bass   { "BASS",   [] (double v) { return juce::String ((int) std::round (v * 100)) + " %"; }, theme::accent2 };
    Knob center { "CENTER", [] (double v) { return juce::String (audiofx::gainToDecibels ((float) v), 1) + " dB"; } };
};

class CompressorPanel : public ModulePanel
{
public:
    explicit CompressorPanel (audiofx::ProcessingChain& chain);
    void resized() override;
    void paint (juce::Graphics&) override;
    void syncFrom (const struct Params& p);
    void refreshMeters();
    std::function<void()> onParamChanged;

private:
    audiofx::ProcessingChain& chain;
    Knob thresh { "THRESH",  [] (double v) { return juce::String (v, 1) + " dB";; }, theme::warn };
    Knob ratio  { "RATIO",   [] (double v) { return juce::String (v, 1) + " : 1"; } };
    Knob attack { "ATTACK",  [] (double v) { return juce::String (v, 1) + " ms"; } };
    Knob release{ "RELEASE", [] (double v) { return juce::String ((int) v) + " ms"; } };
    Knob makeup { "MAKEUP",  [] (double v) { return juce::String (v, 1) + " dB"; }, theme::accent2 };
    LevelMeter grMeter { LevelMeter::Orientation::Vertical };
    juce::Label grLabel { {}, "GR" };
};

class LimiterPanel : public ModulePanel
{
public:
    explicit LimiterPanel (audiofx::ProcessingChain& chain);
    void resized() override;
    void paint (juce::Graphics&) override;
    void syncFrom (const struct Params& p);
    void refreshMeters();
    std::function<void()> onParamChanged;

private:
    audiofx::ProcessingChain& chain;
    Knob ceiling { "CEILING", [] (double v) { return juce::String (v, 1) + " dB"; }, theme::warn };
    Knob release { "RELEASE", [] (double v) { return juce::String ((int) v) + " ms"; } };
    LevelMeter grMeter { LevelMeter::Orientation::Vertical };
    juce::Label grLabel { {}, "GR" };
};

class MasterPanel : public ModulePanel
{
public:
    explicit MasterPanel (audiofx::ProcessingChain& chain);
    void resized() override;
    void paint (juce::Graphics&) override;
    void syncFrom (const struct Params& p);
    void refreshMeters();
    std::function<void()> onParamChanged;

private:
    audiofx::ProcessingChain& chain;
    Knob volume { "MASTER", [] (double v) { return juce::String (v, 1) + " dB"; }, theme::accent3 };
    Knob agcTarget { "TARGET", [] (double v) { return juce::String (v, 0) + " dB"; }, theme::accent };
    juce::ToggleButton agc { "AUTO LEVEL" };
    LevelMeter meterL { LevelMeter::Orientation::Vertical };
    LevelMeter meterR { LevelMeter::Orientation::Vertical };
    juce::Label levelReadout;
};
