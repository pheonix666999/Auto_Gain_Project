#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginLookAndFeel.h"

//==============================================================================
//  Stereo vertical meter with the green -> yellow -> red gradient and a dB
//  scale, matching the IN / OUT meters in the mockup. Levels are 0..1 (already
//  smoothed by the LevelMeter on the audio side).
//==============================================================================
class VerticalMeter : public juce::Component
{
public:
    void setLevels (float l, float r) noexcept
    {
        levelL = l; levelR = r;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (Palette::background.darker(0.2f));
        g.fillRoundedRectangle (bounds, 4.0f);
        
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        auto barsArea = bounds.reduced (3.0f);
        const float gap = 2.0f;
        const float barW = (barsArea.getWidth() - gap) * 0.5f;

        drawBar (g, barsArea.removeFromLeft (barW), levelL);
        barsArea.removeFromLeft (gap);
        drawBar (g, barsArea, levelR);
    }

private:
    void drawBar (juce::Graphics& g, juce::Rectangle<float> area, float level)
    {
        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (area, 2.0f);

        const int   segs   = 22;
        const float segH   = area.getHeight() / (float) segs;
        const int   active = juce::roundToInt (level * segs);

        for (int i = 0; i < segs; ++i)
        {
            const float t = (float) i / (float) (segs - 1);   // 0 bottom .. 1 top
            juce::Colour c;
            if      (t > 0.9f) c = Palette::red;
            else if (t > 0.7f) c = Palette::orange;
            else if (t > 0.5f) c = juce::Colour (0xffd8d33a);  // yellow
            else               c = Palette::green;

            auto seg = juce::Rectangle<float> (area.getX() + 1.0f,
                                               area.getBottom() - (i + 1) * segH + 1.0f,
                                               area.getWidth() - 2.0f,
                                               segH - 1.5f);

            const bool lit = i < active;
            g.setColour (lit ? c : juce::Colour (0xff0a0d14));
            g.fillRect (seg);
        }
    }

    float levelL { 0.0f }, levelR { 0.0f };
};
