// ============================================================================
//  Controls.h - reusable UI atoms: module panel, knob, meters, field viz
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Theme.h"

// ---- module card with title + optional bypass LED ---------------------------
class ModulePanel : public juce::Component
{
public:
    ModulePanel (const juce::String& title, const juce::String& tag = {})
        : panelTitle (title), panelTag (tag)
    {
        addAndMakeVisible (bypass);
        bypass.setButtonText ("ON");
        bypass.setClickingTogglesState (true);
        bypass.setToggleState (true, juce::dontSendNotification);
        bypass.onClick = [this]
        {
            const bool active = bypass.getToggleState();
            bypass.setButtonText (active ? "ON" : "OFF");
            if (onBypassChanged)
                onBypassChanged (! active);
        };
    }

    void paint (juce::Graphics& g) override
    {
        theme::paintGlass (g, getLocalBounds(), panelTitle, panelTag);
    }

    void resized() override
    {
        auto top = getLocalBounds().reduced (12, 8).removeFromTop (24);
        bypass.setBounds (top.removeFromRight (58));
    }

    void setBypassActive (bool isBypassed)   // true = module is bypassed
    {
        bypass.setToggleState (! isBypassed, juce::dontSendNotification);
        bypass.setButtonText (! isBypassed ? "ON" : "OFF");
    }

    std::function<void (bool bypassed)> onBypassChanged;

protected:
    juce::String panelTitle, panelTag;
    juce::ToggleButton bypass;
};

// ---- rotary knob with name + value readout ----------------------------------
class Knob : public juce::Component, private juce::Slider::Listener
{
public:
    using Format = std::function<juce::String (double)>;

    Knob (const juce::String& name, Format fmtIn = nullptr, juce::Colour fill = theme::accent)
        : knobName (name), format (std::move (fmtIn))
    {
        addAndMakeVisible (slider);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f, true);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::rotarySliderFillColourId, fill);
        slider.addListener (this);

        addAndMakeVisible (valueLabel);
        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setFont (theme::monoFont (11.0f, true));
        valueLabel.setColour (juce::Label::textColourId, fill);

        addAndMakeVisible (nameLabel);
        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (theme::font (11.0f));
        nameLabel.setColour (juce::Label::textColourId, theme::textDim);
        nameLabel.setText (knobName, juce::dontSendNotification);
    }

    juce::Slider& getSlider() noexcept { return slider; }

    void setValue (double v, juce::NotificationType n = juce::dontSendNotification)
    {
        slider.setValue (v, n);
        refresh();
    }

    double getValue() const noexcept { return slider.getValue(); }

    void setRange (juce::Range<double> r, double interval = 0.0)
    {
        slider.setRange (r.getStart(), r.getEnd(), interval);
        refresh();
    }

    void setOnValueChange (std::function<void()> f) { onValueChange = std::move (f); }

    void resized() override
    {
        auto area = getLocalBounds();
        valueLabel.setBounds (area.removeFromBottom (14));
        nameLabel.setBounds (area.removeFromBottom (14));
        slider.setBounds (area.reduced (2, 0));
    }

private:
    void sliderValueChanged (juce::Slider*) override
    {
        refresh();
        if (onValueChange)
            onValueChange();
    }

    void refresh()
    {
        const double v = slider.getValue();
        valueLabel.setText (format ? format (v) : juce::String (v, 2), juce::dontSendNotification);
    }

    juce::String knobName;
    Format format;
    juce::Slider slider;
    juce::Label valueLabel, nameLabel;
    std::function<void()> onValueChange;
};

// ---- level meter (vertical, dB scaled) --------------------------------------
class LevelMeter : public juce::Component
{
public:
    enum class Orientation { Vertical, Horizontal };

    explicit LevelMeter (Orientation o = Orientation::Vertical) : orientation (o) {}

    void setLevelDb (float peakDb, float rmsDb = -120.0f)
    {
        peak = juce::jlimit (-60.0f, 6.0f, peakDb);
        rms  = juce::jlimit (-60.0f, 6.0f, rmsDb);
        repaint();
    }

    // for gain-reduction meters (0 = no reduction, positive dB = reducing)
    void setReductionDb (float gr)
    {
        reduction = juce::jlimit (0.0f, 24.0f, gr);
        repaint();
    }

    void setRangeDb (float floorDb, float ceilingDb) { rangeFloor = floorDb; rangeCeiling = ceilingDb; }
    void setAccent (juce::Colour c) { accentColour = c; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (theme::trackDark);
        g.fillRoundedRectangle (b, 3.0f);

        auto bar = b.reduced (1.5f);
        const bool vert = orientation == Orientation::Vertical;

        if (reduction >= 0.0f)
        {
            // GR meter: fills from top downward
            const float t = juce::jlimit (0.0f, 1.0f, reduction / juce::jmax (1.0f, rangeCeiling));
            auto fill = bar.withHeight (bar.getHeight() * t);
            g.setColour (theme::accent2);
            g.fillRoundedRectangle (fill, 2.5f);
            return;
        }

        auto fracFromDb = [this] (float db)
        {
            return juce::jlimit (0.0f, 1.0f, (db - rangeFloor) / (rangeCeiling - rangeFloor));
        };

        const float fp = fracFromDb (peak);
        const float fr = fracFromDb (rms);

        juce::Rectangle<float> fill, rmsFill;
        if (vert)
        {
            fill    = bar.withTop (bar.getBottom() - bar.getHeight() * fp);
            rmsFill = bar.withTop (bar.getBottom() - bar.getHeight() * fr);
        }
        else
        {
            fill    = bar.withWidth (bar.getWidth() * fp);
            rmsFill = bar.withWidth (bar.getWidth() * fr);
        }

        juce::ColourGradient grad (juce::Colour (0xff1FBF6B), vert ? 0.0f : 0.0f,
                                   vert ? (float) getHeight() : 0.0f,
                                   juce::Colour (0xffFFC24A), vert ? 0.0f : (float) getWidth() * 0.55f,
                                   vert ? (float) getHeight() * 0.45f : 0.0f, vert);
        grad.addColour (1.0, juce::Colour (0xffFF5A5A));

        if (vert)
            grad = juce::ColourGradient (juce::Colour (0xffFF5A5A), 0.0f, 0.0f,
                                         juce::Colour (0xff1FBF6B), 0.0f, (float) getHeight(), false);
        else
            grad = juce::ColourGradient (juce::Colour (0xff1FBF6B), 0.0f, 0.0f,
                                         juce::Colour (0xffFF5A5A), (float) getWidth(), 0.0f, false);

        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 2.5f);

        g.setColour (theme::textBright.withAlpha (0.35f));
        if (vert) g.fillRoundedRectangle (rmsFill.withHeight (2.5f).withY (rmsFill.getBottom() - 2.5f), 1.2f);
        else      g.fillRoundedRectangle (rmsFill.withWidth (2.5f), 1.2f);
    }

private:
    Orientation orientation;
    float peak = -60.0f, rms = -60.0f, reduction = -1.0f;
    float rangeFloor = -60.0f, rangeCeiling = 6.0f;
    juce::Colour accentColour = theme::accent;
};

// ---- field visualiser for the surround module -------------------------------
class FieldVisualizer : public juce::Component, private juce::Timer
{
public:
    FieldVisualizer()
    {
        startTimerHz (30);
    }

    void setParams (float amountIn, float spreadIn, float bassIn)
    {
        amount = amountIn; spread = spreadIn; bass = bassIn;
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced (14.0f, 8.0f);
        const auto c = area.getCentre();

        // listening field
        const float w = area.getWidth()  * (0.25f + 0.55f * juce::jlimit (0.0f, 1.0f, spread * 0.5f));
        const float h = area.getHeight() * (0.35f + 0.45f * amount);
        juce::Rectangle<float> field (w, h);
        field.setCentre (c);

        const float pulse = 1.0f + 0.025f * std::sin (phase);
        field = field.expanded (field.getWidth() * (pulse - 1.0f));

        g.setColour (theme::accent.withAlpha (0.05f + 0.12f * amount));
        g.fillEllipse (field);
        g.setColour (theme::accent.withAlpha (0.25f + 0.45f * amount));
        g.drawEllipse (field, 1.4f);

        g.setColour (theme::accent.withAlpha (0.12f + 0.30f * amount));
        g.drawEllipse (field.expanded (6.0f), 1.0f);

        // speakers
        const float sp = 7.0f;
        auto drawSpeaker = [&] (juce::Point<float> p, juce::Colour col)
        {
            g.setColour (col.withAlpha (0.35f));
            g.fillEllipse (p.x - sp, p.y - sp, sp * 2.4f, sp * 2.4f);
            g.setColour (col);
            g.fillEllipse (p.x - sp * 0.5f, p.y - sp * 0.5f, sp, sp);
        };

        drawSpeaker ({ field.getX() - 4.0f, field.getY() + 8.0f }, theme::accent);
        drawSpeaker ({ field.getRight() + 4.0f, field.getY() + 8.0f }, theme::accent);

        // listener
        g.setColour (theme::accent3);
        g.fillEllipse (c.x - 4.0f, c.y + field.getHeight() * 0.32f - 4.0f, 8.0f, 8.0f);
        g.setColour (theme::accent3.withAlpha (0.30f));
        g.fillEllipse (c.x - 8.0f, c.y + field.getHeight() * 0.32f - 8.0f, 16.0f, 16.0f);

        // bass core
        const float core = 8.0f + 14.0f * bass;
        g.setColour (theme::accent2.withAlpha (0.10f + 0.25f * bass));
        g.fillEllipse (c.x - core, c.y + field.getHeight() * 0.32f - core, core * 2, core * 2);
    }

private:
    void timerCallback() override { phase += 0.14f; repaint(); }

    float amount = 0.35f, spread = 1.15f, bass = 0.7f, phase = 0.0f;
};
