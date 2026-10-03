// ============================================================================
//  Theme.h - modern dark-glass visual language
// ============================================================================
#pragma once

#include <JuceHeader.h>

namespace theme
{
    // ---- palette -------------------------------------------------------------
    const juce::Colour bg0        { 0xff070B10 };   // deep space
    const juce::Colour bg1        { 0xff0D131C };
    const juce::Colour panelEdge  { 0x22FFFFFF };
    const juce::Colour panelFill  { 0x14FFFFFF };   // glass
    const juce::Colour textBright { 0xffEAF2FA };
    const juce::Colour textDim    { 0xff7F8C9B };
    const juce::Colour textFaint  { 0xff4A5563 };
    const juce::Colour accent     { 0xff33E1FF };   // electric cyan
    const juce::Colour accent2    { 0xffFF3D9A };   // magenta
    const juce::Colour accent3    { 0xff5DFF9B };   // mint (AGC / ok)
    const juce::Colour warn       { 0xffFFC24A };
    const juce::Colour danger     { 0xffFF5A5A };
    const juce::Colour trackDark  { 0xFF1A222D };
    const juce::Colour glassHi    { 0x2AFFFFFF };

    inline juce::Font font (float size, bool bold = false)
    {
        juce::Font f { juce::FontOptions { size } };
        return bold ? f.boldened() : f;
    }

    inline juce::Font monoFont (float size, bool bold = false)
    {
        #if JUCE_WINDOWS
        juce::Font f { juce::FontOptions { "Consolas", size, juce::Font::plain } };
        #else
        juce::Font f { juce::FontOptions { size } };
        #endif
        return bold ? f.boldened() : f;
    }

    // glass card with title
    inline void paintGlass (juce::Graphics& g, juce::Rectangle<int> area,
                            const juce::String& title, const juce::String& tag = {})
    {
        auto b = area.toFloat().reduced (4.0f);
        g.setColour (panelFill);
        g.fillRoundedRectangle (b, 16.0f);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (b, 16.0f, 1.0f);

        // top highlight
        juce::Path p;
        p.addRoundedRectangle (b.withHeight (b.getHeight() * 0.45f), 16.0f);
        g.setColour (glassHi);
        g.fillPath (p);

        auto header = area.reduced (18, 14).removeFromTop (20);
        g.setColour (textBright);
        g.setFont (font (13.5f, true));
        g.drawText (title, header, juce::Justification::centredLeft, false);

        if (tag.isNotEmpty())
        {
            g.setColour (textFaint);
            g.setFont (monoFont (10.5f));
            g.drawText (tag, header, juce::Justification::centredRight, false);
        }
    }

    inline void paintBackdrop (juce::Graphics& g, juce::Rectangle<int> area)
    {
        juce::ColourGradient grad (bg0, 0.0f, 0.0f, bg1, 0.0f, (float) area.getHeight(), false);
        g.setGradientFill (grad);
        g.fillAll();

        // faint horizon glow
        juce::ColourGradient glow (accent.withAlpha (0.05f), (float) area.getCentreX(), (float) area.getBottom(),
                                   bg0.withAlpha (0.0f), (float) area.getCentreX(), (float) area.getCentreY(), true);
        g.setGradientFill (glow);
        g.fillAll();
    }
}
