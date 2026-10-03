// ============================================================================
//  LookAndFeel.cpp
// ============================================================================
#include "LookAndFeel.h"
#include "Theme.h"

ThemedLookAndFeel::ThemedLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, theme::bg0);
    setColour (juce::Slider::textBoxTextColourId, theme::textDim);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::trackColourId, theme::accent);
    setColour (juce::Slider::thumbColourId, theme::textBright);
    setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, theme::accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, theme::trackDark);

    setColour (juce::ComboBox::backgroundColourId, theme::trackDark);
    setColour (juce::ComboBox::textColourId, theme::textBright);
    setColour (juce::ComboBox::outlineColourId, theme::panelEdge);
    setColour (juce::ComboBox::arrowColourId, theme::accent);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xF0121821));
    setColour (juce::PopupMenu::textColourId, theme::textBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, theme::textBright);

    setColour (juce::Label::textColourId, theme::textDim);
    setColour (juce::TextButton::buttonColourId, theme::trackDark);
    setColour (juce::TextButton::textColourOffId, theme::textBright);
    setColour (juce::TextButton::textColourOnId, theme::bg0);
    setColour (juce::ToggleButton::textColourId, theme::textDim);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xF0121821));
}

// ------------------------------------------------------------------ knob ----
void ThemedLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float arcThickness = juce::jmax (2.5f, radius * 0.12f);
    const float arcRadius = radius - arcThickness * 0.5f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (theme::trackDark);
    g.strokePath (track, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (angle > rotaryStartAngle + 0.0001f)
    {
        juce::Path val;
        val.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                           rotaryStartAngle, angle, true);

        const auto c = slider.findColour (juce::Slider::rotarySliderFillColourId);
        g.setColour (c.withAlpha (0.25f));       // glow pass
        g.strokePath (val, juce::PathStrokeType (arcThickness * 2.2f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour (c);
        g.strokePath (val, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    // knob body: dark glass sphere with a subtle rim
    const float bodyRadius = arcRadius - arcThickness * 1.4f;
    if (bodyRadius > 4.0f)
    {
        const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

        juce::ColourGradient grad (juce::Colour (0xFF2A3442), centre.x, body.getY(),
                                   juce::Colour (0xFF12181F), centre.x, body.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (body);
        g.setColour (theme::panelEdge);
        g.drawEllipse (body.reduced (0.5f), 1.0f);

        // pointer
        const float pointerLen = bodyRadius * 0.62f;
        const float pointerThickness = juce::jmax (2.0f, bodyRadius * 0.10f);
        juce::Path pointer;
        pointer.addRoundedRectangle (-pointerThickness * 0.5f, -bodyRadius + 3.0f,
                                     pointerThickness, pointerLen, pointerThickness * 0.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.fillPath (pointer);
    }
}

// ----------------------------------------------------------------- fader ----
void ThemedLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float, float,
                                          juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool vertical = style == juce::Slider::LinearVertical;
    const auto fill = slider.findColour (juce::Slider::trackColourId);

    if (vertical)
    {
        const float trackW = 6.0f;
        const float cx = (float) x + (float) width * 0.5f;
        const float top = (float) y + 4.0f, bot = (float) y + (float) height - 4.0f;

        juce::Rectangle<float> track (cx - trackW * 0.5f, top, trackW, bot - top);
        g.setColour (theme::trackDark);
        g.fillRoundedRectangle (track, trackW * 0.5f);

        // fill from centre (0 dB) to handle
        const float zero = top + (bot - top) * (1.0f - 0.5f);   // 0 dB at mid for +/- range
        const float handleY = juce::jlimit (top, bot, sliderPos);
        const float a = juce::jmin (zero, handleY), b = juce::jmax (zero, handleY);
        g.setColour (fill);
        g.fillRoundedRectangle (cx - trackW * 0.5f, a, trackW, b - a, trackW * 0.5f);

        // glass cap
        const float capH = 13.0f, capW = 30.0f;
        juce::Rectangle<float> cap (cx - capW * 0.5f, handleY - capH * 0.5f, capW, capH);
        g.setColour (juce::Colour (0xFF2C3846));
        g.fillRoundedRectangle (cap, 4.0f);
        g.setColour (theme::panelEdge);
        g.drawRoundedRectangle (cap, 4.0f, 1.0f);
        g.setColour (fill);
        g.fillRoundedRectangle (cap.withSizeKeepingCentre (capW - 10.0f, 2.5f), 1.2f);
    }
    else   // horizontal (meters use this style too)
    {
        const float trackH = 5.0f;
        const float cy = (float) y + (float) height * 0.5f;
        const float left = (float) x + 3.0f, right = (float) x + (float) width - 3.0f;

        juce::Rectangle<float> track (left, cy - trackH * 0.5f, right - left, trackH);
        g.setColour (theme::trackDark);
        g.fillRoundedRectangle (track, trackH * 0.5f);

        const float handleX = juce::jlimit (left, right, sliderPos);
        g.setColour (fill);
        g.fillRoundedRectangle (left, cy - trackH * 0.5f, handleX - left, trackH, trackH * 0.5f);

        const float capW = 12.0f, capH = 24.0f;
        juce::Rectangle<float> cap (handleX - capW * 0.5f, cy - capH * 0.5f, capW, capH);
        g.setColour (juce::Colour (0xFF2C3846));
        g.fillRoundedRectangle (cap, 3.5f);
        g.setColour (theme::panelEdge);
        g.drawRoundedRectangle (cap, 3.5f, 1.0f);
    }
}

// ---------------------------------------------------------------- button ----
void ThemedLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour&, bool highlighted, bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced (1.0f);
    const float r = b.getHeight() * 0.42f;

    juce::Colour base = button.getToggleState() ? theme::accent : juce::Colour (0xFF1E2833);
    if (down)          base = base.darker (0.15f);
    else if (highlighted) base = base.brighter (0.12f);

    if (button.getToggleState())
    {
        g.setColour (theme::accent.withAlpha (0.30f));
        g.fillRoundedRectangle (b.expanded (1.5f), r + 1.5f);
    }
    g.setColour (base);
    g.fillRoundedRectangle (b, r);
    g.setColour (theme::panelEdge);
    g.drawRoundedRectangle (b, r, 1.0f);
}

void ThemedLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool highlighted, bool)
{
    auto area = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    auto led = area.removeFromLeft (juce::jmin (18.0f, area.getWidth() * 0.35f)).reduced (3.0f);
    auto labelArea = area;

    const auto ledColour = on ? theme::accent3 : theme::textFaint;
    if (on)
    {
        g.setColour (ledColour.withAlpha (0.35f));
        g.fillEllipse (led.expanded (2.5f));
    }
    g.setColour (ledColour);
    g.fillEllipse (led);
    g.setColour (theme::panelEdge);
    g.drawEllipse (led, 1.0f);

    g.setColour (highlighted ? theme::textBright : (on ? theme::textBright : theme::textDim));
    g.setFont (theme::font (11.5f, true));
    g.drawText (button.getButtonText(), labelArea.toNearestInt(), juce::Justification::centredLeft, false);
}

// --------------------------------------------------------------- combo ------
void ThemedLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                      int, int, int, int, juce::ComboBox& box)
{
    auto b = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
    g.setColour (box.isEnabled() ? juce::Colour (0xFF1A222D) : juce::Colour (0xFF141A22));
    g.fillRoundedRectangle (b, 9.0f);
    g.setColour (theme::panelEdge);
    g.drawRoundedRectangle (b, 9.0f, 1.0f);

    juce::Path chevron;
    const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
    chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
    chevron.lineTo (cx, cy + 2.5f);
    chevron.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (theme::accent);
    g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

void ThemedLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 32, box.getHeight() - 2);
    label.setFont (theme::font (12.5f));
}

void ThemedLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (theme::panelEdge);
    g.drawRect (0, 0, width, height, 1);
}

void ThemedLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                           bool isSeparator, bool isActive, bool isHighlighted,
                                           bool isTicked, bool, const juce::String& text,
                                           const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (theme::panelEdge);
        g.drawHorizontalLine (area.getCentreY(), (float) area.getX() + 8.0f, (float) area.getRight() - 8.0f);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRoundedRectangle (area.toFloat().reduced (3.0f), 6.0f);
    }

    g.setColour (isActive ? (isHighlighted ? theme::textBright : theme::textBright)
                          : theme::textFaint);
    g.setFont (theme::font (12.5f));
    auto r = area.reduced (12, 4);
    if (isTicked)
    {
        g.setColour (theme::accent);
        g.fillEllipse ((float) r.getX() - 2.0f, (float) r.getCentreY() - 2.5f, 5.0f, 5.0f);
    }
    g.drawText (text, r, juce::Justification::centredLeft, true);
}

juce::Font ThemedLookAndFeel::getComboBoxFont (juce::ComboBox&)  { return theme::font (12.5f); }
juce::Font ThemedLookAndFeel::getPopupMenuFont()                 { return theme::font (12.5f); }
juce::Font ThemedLookAndFeel::getLabelFont (juce::Label&)        { return theme::font (12.5f); }
juce::Font ThemedLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return theme::font (12.5f, true); }
