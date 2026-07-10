#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class VacuumTube : public juce::Component
{
public:
    void setGlowLevel (float level) noexcept
    {
        glowLevel = juce::jlimit (0.0f, 1.0f, level);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (8.0f);
        
        // base
        float baseH = bounds.getHeight() * 0.15f;
        auto baseRect = juce::Rectangle<float> (bounds.getX() + bounds.getWidth() * 0.15f,
                                                bounds.getBottom() - baseH,
                                                bounds.getWidth() * 0.7f,
                                                baseH);
        
        g.setColour (juce::Colour (0xff151515));
        g.fillRoundedRectangle (baseRect, 3.0f);
        g.setColour (juce::Colour (0xff333333));
        g.drawRoundedRectangle (baseRect, 3.0f, 1.5f);

        // glass envelope
        auto bulbRect = juce::Rectangle<float> (bounds.getX() + bounds.getWidth() * 0.2f,
                                                bounds.getY(),
                                                bounds.getWidth() * 0.6f,
                                                bounds.getHeight() - baseH - 4.0f);
        
        // draw shadow inside
        g.setColour (juce::Colour (0x44000000));
        g.fillRoundedRectangle (bulbRect, bulbRect.getWidth() * 0.4f);

        // grid / anode internals
        float internalsW = bulbRect.getWidth() * 0.4f;
        auto anodeRect = juce::Rectangle<float> (bulbRect.getCentreX() - internalsW * 0.5f,
                                                 bulbRect.getY() + bulbRect.getHeight() * 0.25f,
                                                 internalsW,
                                                 bulbRect.getHeight() * 0.5f);
        
        g.setColour (juce::Colour (0xff252525));
        g.fillRoundedRectangle (anodeRect, 2.0f);
        g.setColour (juce::Colour (0xff121212));
        g.drawRoundedRectangle (anodeRect, 2.0f, 1.0f);

        // filament glow (orange / yellow glow in the center)
        float glowAmount = 0.2f + 0.8f * glowLevel;
        auto glowColour = juce::Colour (0xffff6a00).interpolatedWith (juce::Colour (0xffffcc00), glowLevel);

        // Draw dynamic glow layers
        for (int i = 8; i > 0; --i)
        {
            g.setColour (glowColour.withAlpha (glowAmount * 0.08f * (9 - i)));
            g.drawRoundedRectangle (anodeRect.expanded ((float)i * 2.0f), 4.0f, (float)i);
        }

        // draw logo icon inside anode
        g.setColour (glowColour.withAlpha (0.4f + glowAmount * 0.5f));
        g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
        g.drawText ("T", anodeRect, juce::Justification::centred);

        // draw glass highlights (white specular lines)
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (bulbRect, bulbRect.getWidth() * 0.4f, 1.5f);
        
        // left vertical specular highlight
        juce::Path highlight;
        highlight.addCentredArc (bulbRect.getCentreX(), bulbRect.getY() + bulbRect.getWidth() * 0.4f, 
                                 bulbRect.getWidth() * 0.35f, bulbRect.getWidth() * 0.35f, 
                                 0.0f, juce::degreesToRadians (180.0f), juce::degreesToRadians (270.0f), true);
        highlight.lineTo (bulbRect.getX() + bulbRect.getWidth() * 0.15f, bulbRect.getBottom() - 10.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.strokePath (highlight, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

private:
    float glowLevel { 0.0f };
};
