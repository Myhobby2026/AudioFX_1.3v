// ============================================================================
//  PresetManager.h - factory + user presets, plus settings persistence
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Params.h"

class PresetManager
{
public:
    PresetManager();

    // ---- factory presets (baked in) -------------------------------------------
    static juce::StringArray factoryPresetNames();
    static bool getFactoryPreset (const juce::String& name, Params& out);

    // ---- user presets (XML files in %APPDATA%/AudioFX/Presets) ----------------
    juce::StringArray userPresetNames() const;
    bool saveUserPreset (const juce::String& name, const Params& p);
    bool deleteUserPreset (const juce::String& name);
    bool getUserPreset (const juce::String& name, Params& out) const;

    juce::StringArray allPresetNames() const;

    // ---- app settings ---------------------------------------------------------
    void saveSettings (const Params& p);
    bool loadSettings (Params& p);

    juce::File getPresetDirectory() const;
    juce::File getSettingsFile() const;
};
