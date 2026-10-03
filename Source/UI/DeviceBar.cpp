// ============================================================================
//  DeviceBar.cpp
// ============================================================================
#include "DeviceBar.h"
#include "BinaryData.h"

DeviceBar::DeviceBar()
{
    if (auto img = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize); img.isValid())
        logo = img.rescaled (44, 44, juce::Graphics::highResamplingQuality);

    for (auto* c : { &modeBox, &inputBox, &outputBox, &presetBox })
    {
        addAndMakeVisible (*c);
        c->setJustificationType (juce::Justification::centredLeft);
    }

    modeBox.onChange   = [this] { if (onModeChanged)     onModeChanged (modeBox.getSelectedId()); };
    inputBox.onChange  = [this] { if (onInputChanged)    onInputChanged(); };
    outputBox.onChange = [this] { if (onOutputChanged)   onOutputChanged(); };
    presetBox.onChange = [this] { if (onPresetChanged)   onPresetChanged(); };

    addAndMakeVisible (savePreset);
    addAndMakeVisible (deletePreset);
    savePreset.onClick   = [this] { if (onSavePreset)   onSavePreset(); };
    deletePreset.onClick = [this] { if (onDeletePreset) onDeletePreset(); };

    addAndMakeVisible (globalBypass);
    globalBypass.setClickingTogglesState (true);
    globalBypass.onClick = [this] { if (onGlobalBypass) onGlobalBypass (globalBypass.getToggleState()); };

    status.setFont (theme::monoFont (10.5f));
    status.setColour (juce::Label::textColourId, theme::textDim);
    status.setJustificationType (juce::Justification::centredRight);
    status.setText ("starting...", juce::dontSendNotification);
    addAndMakeVisible (status);
}

void DeviceBar::setStatus (bool streaming, double sampleRate, int blockSize)
{
    if (streaming)
        status.setText (juce::String (sampleRate / 1000.0, 1) + " kHz  /  " + juce::String (blockSize) + " smp",
                        juce::dontSendNotification);
    else
        status.setText ("idle", juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, streaming ? theme::accent3 : theme::textFaint);
}

void DeviceBar::setModeItems (const juce::StringArray& items, int selectedId)
{
    modeBox.clear (juce::dontSendNotification);
    int id = 1;
    for (const auto& m : items)
        modeBox.addItem (m, id++);
    modeBox.setSelectedId (selectedId, juce::dontSendNotification);
}

void DeviceBar::setInputItems (const juce::StringArray& devices, const juce::String& selected)
{
    inputBox.clear (juce::dontSendNotification);
    inputBox.setTextWhenNothingSelected ("- capture device -");
    int id = 1;
    for (const auto& d : devices)
        inputBox.addItem (d, id++);
    inputBox.setText (selected, juce::dontSendNotification);
}

void DeviceBar::setOutputItems (const juce::StringArray& devices, const juce::String& selected)
{
    outputBox.clear (juce::dontSendNotification);
    outputBox.setTextWhenNothingSelected ("- output device -");
    int id = 1;
    for (const auto& d : devices)
        outputBox.addItem (d, id++);
    outputBox.setText (selected, juce::dontSendNotification);
}

void DeviceBar::setPresetItems (const juce::StringArray& names, const juce::String& selected)
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected ("- preset -");
    int id = 1;
    for (const auto& n : names)
        presetBox.addItem (n, id++);
    presetBox.setText (selected, juce::dontSendNotification);
}

void DeviceBar::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (theme::panelFill);
    g.fillRoundedRectangle (area.reduced (4.0f), 18.0f);
    g.setColour (theme::panelEdge);
    g.drawRoundedRectangle (area.reduced (4.0f), 18.0f, 1.0f);

    auto left = area.reduced (18.0f, 10.0f);
    if (logo.isValid())
        g.drawImage (logo, left.removeFromLeft (44.0f));

    left.removeFromLeft (10.0f);
    auto textArea = left.removeFromLeft (150.0f);
    g.setColour (theme::textBright);
    g.setFont (theme::font (19.0f, true));
    g.drawText ("AudioFX", textArea.removeFromTop (24.0f), juce::Justification::centredLeft, false);
    g.setColour (theme::textFaint);
    g.setFont (theme::monoFont (9.5f));
    g.drawText ("SYSTEM-WIDE DSP", textArea, juce::Justification::centredLeft, false);
}

void DeviceBar::resized()
{
    auto area = getLocalBounds().reduced (14, 12);

    status.setBounds (area.removeFromRight (130));
    area.removeFromRight (10);

    globalBypass.setBounds (area.removeFromRight (86));
    area.removeFromRight (10);

    deletePreset.setBounds (area.removeFromRight (44));
    area.removeFromRight (4);
    savePreset.setBounds (area.removeFromRight (54));
    area.removeFromRight (6);
    presetBox.setBounds (area.removeFromRight (150));
    area.removeFromRight (8);

    // skip the branding block on the left
    area.removeFromLeft (218);

    outputBox.setBounds (area.removeFromRight (180));
    area.removeFromRight (6);
    inputBox.setBounds (area.removeFromRight (210));
    area.removeFromRight (6);
    modeBox.setBounds (area.removeFromRight (170));
}
