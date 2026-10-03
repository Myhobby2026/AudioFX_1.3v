// ============================================================================
//  PresetManager.cpp
// ============================================================================
#include "PresetManager.h"

PresetManager::PresetManager()
{
    getPresetDirectory().createDirectory();
}

juce::File PresetManager::getPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("AudioFX")
        .getChildFile ("Presets");
}

juce::File PresetManager::getSettingsFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("AudioFX")
        .getChildFile ("settings.xml");
}

// ------------------------------------------------------------- factory ------
juce::StringArray PresetManager::factoryPresetNames()
{
    return { "Flat", "Bass Boost", "Vocal Clarity", "Cinema Field", "Night Mode", "Studio Ref" };
}

bool PresetManager::getFactoryPreset (const juce::String& name, Params& out)
{
    out = Params {};

       if (name == "Flat")
        return true;

    if (name == "Bass Boost")
    {
        const float g[10] = { 7, 6, 4, 1, 0, 0, 0, 0, 0, 0 };
        for (int i = 0; i < 10; ++i) out.eqGainDb[i] = g[i];
        out.surBass = 0.9f;
        return true;
    }

    if (name == "Vocal Clarity")
    {
        const float g[10] = { -3, -2, -1, 0, 2, 4, 4, 2, 1, 0 };
        for (int i = 0; i < 10; ++i) out.eqGainDb[i] = g[i];
        out.revWet = 0.10f;
        out.surAmount = 0.2f;
        return true;
    }

    if (name == "Cinema Field")
    {
        const float g[10] = { 4, 3, 1, 0, 0, 0, 1, 2, 2, 3 };
        for (int i = 0; i < 10; ++i) out.eqGainDb[i] = g[i];
        out.revRoom = 0.70f; out.revWet = 0.35f; out.revWidth = 1.0f;
        out.surAmount = 0.75f; out.surSpread = 1.5f; out.surBass = 0.8f;
        out.limCeilingDb = -1.0f;
        return true;
    }

    if (name == "Night Mode")
    {
        const float g[10] = { 3, 2, 0, -1, -1, 0, 1, 1, -1, -3 };
        for (int i = 0; i < 10; ++i) out.eqGainDb[i] = g[i];
        out.volDb = -12.0f;
        out.agcEnabled = true; out.agcTargetDb = -24.0f;
        out.limCeilingDb = -6.0f;
        out.revWet = 0.15f;
        return true;
    }

    if (name == "Studio Ref")
    {
        out.revWet = 0.0f;
        out.surAmount = 0.0f; out.surSpread = 1.0f;
        out.agcEnabled = false;
        out.limCeilingDb = -0.3f;
        return true;
    }

    return false;
}

// --------------------------------------------------------------- user -------
juce::StringArray PresetManager::userPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool PresetManager::saveUserPreset (const juce::String& name, const Params& p)
{
    if (name.isEmpty())
        return false;
    auto file = getPresetDirectory().getChildFile (name + ".xml");
    std::unique_ptr<juce::XmlElement> xml (p.toXml());
    return file.replaceWithText (xml->toString());
}

bool PresetManager::deleteUserPreset (const juce::String& name)
{
    auto file = getPresetDirectory().getChildFile (name + ".xml");
    return file.existsAsFile() && file.deleteFile();
}

bool PresetManager::getUserPreset (const juce::String& name, Params& out) const
{
    auto file = getPresetDirectory().getChildFile (name + ".xml");
    if (! file.existsAsFile())
        return false;
    if (auto xml = juce::parseXML (file.loadFileAsString()))
    {
        out = Params {};
        out.fromXml (*xml);
        return true;
    }
    return false;
}

juce::StringArray PresetManager::allPresetNames() const
{
    juce::StringArray names = factoryPresetNames();
    for (const auto& n : userPresetNames())
        if (! names.contains (n))
            names.add (n);
    return names;
}

// ------------------------------------------------------------- settings -----
void PresetManager::saveSettings (const Params& p)
{
    std::unique_ptr<juce::XmlElement> xml (p.toXml());
    getSettingsFile().getParentDirectory().createDirectory();
    getSettingsFile().replaceWithText (xml->toString());
}

bool PresetManager::loadSettings (Params& p)
{
    auto f = getSettingsFile();
    if (! f.existsAsFile())
        return false;
    if (auto xml = juce::parseXML (f.loadFileAsString()))
    {
        p.fromXml (*xml);
        return true;
    }
    return false;
}
