#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
//  Premium palette for the Tractone "WapDem Saturation" mockup.
//==============================================================================
namespace Palette
{
    const juce::Colour background  { 0xff0a0d14 }; // Very dark slate blue-black
    const juce::Colour panel       { 0xff121620 }; // Dark panel background
    const juce::Colour panelEdge   { 0xff1e2433 }; // Subtle border
    const juce::Colour gold        { 0xffeab308 }; // Yellow accent
    const juce::Colour goldBright  { 0xfffde047 }; // Bright yellow
    const juce::Colour green       { 0xff84cc16 }; // Lime/Green
    const juce::Colour greenBright { 0xffbef264 }; // Bright lime
    const juce::Colour orange      { 0xfff97316 }; // Orange
    const juce::Colour red         { 0xffef4444 }; // Bypass Red
    const juce::Colour textGrey    { 0xff94a3b8 }; // Slate text
    const juce::Colour textBright  { 0xffe2e8f0 }; // White-slate text
    const juce::Colour textDim     { 0xff475569 }; // Muted slate text
}

//==============================================================================
//  Custom LookAndFeel matching the premium dark theme, with distinct color
//  knob value arches and custom styled dropdowns/buttons.
//==============================================================================
class PluginLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PluginLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, Palette::panel);
        setColour (juce::ComboBox::outlineColourId,    Palette::panelEdge);
        setColour (juce::ComboBox::textColourId,       Palette::textBright);
        setColour (juce::ComboBox::arrowColourId,      Palette::gold);
        setColour (juce::PopupMenu::backgroundColourId, Palette::panel);
        setColour (juce::PopupMenu::textColourId,       Palette::textBright);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::gold.withAlpha (0.2f));
    }

    //==========================================================================
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        const auto centre = bounds.getCentre();
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        const float arcThickness = radius * 0.16f;
        const float arcRadius    = radius - arcThickness * 0.5f;

        // --- background track ---
        {
            juce::Path bg;
            bg.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (juce::Colour (0xff090b0e));
            g.strokePath (bg, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        // --- value arc (glowing green/yellow or orange/yellow indicator arc) ---
        {
            juce::Path arc;
            arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, angle, true);
            
            juce::Colour cStart = Palette::green;
            juce::Colour cEnd = Palette::greenBright;

            const auto cid = slider.getComponentID();
            if (cid == "drive" || cid == "output" || cid == "punch")
            {
                cStart = Palette::orange;
                cEnd = Palette::goldBright;
            }
            else if (cid == "width" || cid == "mix")
            {
                cStart = Palette::green;
                cEnd = Palette::gold;
            }
            
            const juce::Colour c = cStart.interpolatedWith (cEnd, sliderPos);
            
            // Draw main value arc
            g.setColour (c);
            g.strokePath (arc, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

            // Outer glow effect
            g.setColour (c.withAlpha (0.15f));
            g.strokePath (arc, juce::PathStrokeType (arcThickness * 1.8f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        // --- knob body (brushed dark metal) ---
        const float bodyR = radius - arcThickness - 3.0f;
        const auto bodyBounds = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (centre);

        juce::ColourGradient grad (juce::Colour (0xff1b202c), bodyBounds.getTopLeft(),
                                   juce::Colour (0xff0d0f14), bodyBounds.getBottomRight(), false);
        g.setGradientFill (grad);
        g.fillEllipse (bodyBounds);

        g.setColour (juce::Colour (0xff252e3f));
        g.drawEllipse (bodyBounds, 1.2f);

        // --- indicator pointer dot on the edge of the knob ---
        {
            const float pr = bodyR * 0.72f;
            const float dotSize = bodyR * 0.18f;
            float px = centre.x + pr * std::sin (angle);
            float py = centre.y - pr * std::cos (angle);
            
            g.setColour (Palette::goldBright);
            g.fillEllipse (px - dotSize * 0.5f, py - dotSize * 0.5f, dotSize, dotSize);
        }

        // --- Value text inside knob while dragging ---
        if (slider.isMouseButtonDown())
        {
            juce::String valueText = slider.getTextFromValue (slider.getValue());
            const float fontSize = juce::jmax (9.0f, bodyR * 0.38f);
            g.setFont (juce::Font (juce::FontOptions (fontSize, juce::Font::bold)));
            g.setColour (Palette::textBright);
            g.drawText (valueText, bodyBounds.toNearestInt(), juce::Justification::centred, false);
        }
    }

    //==========================================================================
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
        
        // Background
        g.setColour (Palette::panel);
        g.fillRoundedRectangle (bounds, 5.0f);
        
        // Border
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, 5.0f, 1.2f);

        // Arrow
        const float arrowX = (float) width - 16.0f;
        juce::Path arrow;
        arrow.addTriangle (arrowX, height * 0.45f, arrowX + 8.0f, height * 0.45f,
                           arrowX + 4.0f, height * 0.58f);
        g.setColour (Palette::gold);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.0f, juce::Font::plain));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (10, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::left);
        label.setColour (juce::Label::textColourId, Palette::textBright);
    }

    //==========================================================================
    void drawTickBox (juce::Graphics& g, juce::Component&,
                      float x, float y, float w, float h,
                      bool ticked, bool, bool, bool) override
    {
        const float r = juce::jmin (w, h) * 0.65f;
        const auto bounds = juce::Rectangle<float> (r, r).withCentre ({ x + w * 0.5f, y + h * 0.5f });
        
        g.setColour (Palette::panel);
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (bounds, 3.0f, 1.2f);

        if (ticked)
        {
            g.setColour (Palette::green);
            g.fillRoundedRectangle (bounds.reduced (2.0f), 2.0f);

            // Glow effect
            g.setColour (Palette::green.withAlpha (0.2f));
            g.drawRoundedRectangle (bounds.expanded (1.0f), 3.0f, 1.0f);
        }
    }
};
