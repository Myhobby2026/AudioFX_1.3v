// ============================================================================
//  MainComponent.h - top-level layout: device bar + spectrum + EQ + FX + master
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "AudioEngine.h"
#include "State/PresetManager.h"
#include "UI/LookAndFeel.h"
#include "UI/DeviceBar.h"
#include "UI/SpectrumComponent.h"
#include "UI/EQPanel.h"
#include "UI/ModulePanels.h"

class MainComponent : public juce::Component, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshDeviceLists();
    void refreshPresetList (const juce::String& select = {});
    void loadParams (const Params& p);
    void markDirty();
    void promptSavePreset();

    AudioEngine engine;
    PresetManager presetManager;
    ThemedLookAndFeel lookAndFeel;

    void showFx (int idx);

    DeviceBar deviceBar;
    SpectrumComponent spectrum { engine.getChain() };
    EQPanel eqPanel { engine.getChain() };

    // FX tab bar: REVERB | FIELD | COMPRESSOR | LIMITER
    juce::TextButton tabReverb  { "REVERB" };
    juce::TextButton tabField   { "FIELD" };
    juce::TextButton tabComp    { "COMP" };
    juce::TextButton tabLimiter { "LIMITER" };
    int activeFx = 2;

    ReverbPanel reverbPanel { engine.getChain() };
    SurroundPanel surroundPanel { engine.getChain() };
    CompressorPanel compPanel { engine.getChain() };
    LimiterPanel limiterPanel { engine.getChain() };
    MasterPanel masterPanel { engine.getChain() };

    Params params;
    juce::String currentPresetName { "Flat" };
    bool settingsDirty = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
