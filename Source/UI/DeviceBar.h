// ============================================================================
//  DeviceBar.h - brand header + device routing + presets + global bypass
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class DeviceBar : public juce::Component
{
public:
    DeviceBar();

    void paint (juce::Graphics&) override;
    void resized() override;

    void setStatus (bool streaming, double sampleRate, int blockSize);
    void setModeItems (const juce::StringArray& items, int selectedId);
    void setInputItems (const juce::StringArray& devices, const juce::String& selected);
    void setOutputItems (const juce::StringArray& devices, const juce::String& selected);
    void setPresetItems (const juce::StringArray& names, const juce::String& selected);

    juce::ComboBox modeBox, inputBox, outputBox, presetBox;
    juce::TextButton savePreset { "SAVE" }, deletePreset { "DEL" };
    juce::ToggleButton globalBypass { "BYPASS" };

    std::function<void (int)>  onModeChanged;
    std::function<void()>      onInputChanged, onOutputChanged, onPresetChanged, onSavePreset, onDeletePreset;
    std::function<void (bool)> onGlobalBypass;

private:
    juce::Label status;
    juce::Image logo;
};
