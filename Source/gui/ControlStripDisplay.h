#pragma once

#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Parameters.h"
#include "PluginLookAndFeel.h"

//==============================================================================
// Compact display for the bottom control strip: low-end preservation, harmonics,
// width, and stereo link. This fills the engine panel without adding new DSP.
//==============================================================================
class ControlStripDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ControlStripDisplay (juce::AudioProcessorValueTreeState& state)
        : apvts (state)
    {
        startTimerHz (24);
    }

    ~ControlStripDisplay() override
    {
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        juce::ColourGradient bg (juce::Colour (0xff070a10), bounds.getTopLeft(),
                                 juce::Colour (0xff121722), bounds.getBottomRight(), false);
        bg.addColour (0.65, juce::Colour (0xff0b1018));
        g.setGradientFill (bg);
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

        auto area = bounds.reduced (10.0f, 8.0f);
        g.setColour (Palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText ("CONTROL STRIP MONITOR", area.removeFromTop (12.0f).toNearestInt(), juce::Justification::left);

        area.removeFromTop (4.0f);

        auto eqArea = area.removeFromLeft (155.0f);
        area.removeFromLeft (12.0f);
        auto harmonicsArea = area.removeFromLeft (88.0f);
        area.removeFromLeft (12.0f);
        auto widthArea = area;

        drawEq (g, eqArea);
        drawHarmonics (g, harmonicsArea);
        drawWidth (g, widthArea);
    }

private:
    void timerCallback() override
    {
        repaint();
    }

    void drawEq (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const float freq = apvts.getRawParameterValue (ParamID::crossoverFreq)->load();
        const int band = (int) apvts.getRawParameterValue (ParamID::bandSelect)->load();

        g.setColour (Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText ("LOW END", area.removeFromTop (12.0f).toNearestInt(), juce::Justification::left);

        auto graph = area.reduced (0.0f, 3.0f);
        g.setColour (juce::Colour (0xff05070b));
        g.fillRoundedRectangle (graph, 5.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (graph, 5.0f, 1.0f);

        const float split = juce::jmap (freq, 20.0f, 250.0f, graph.getX() + 14.0f, graph.getRight() - 14.0f);
        const float midY = graph.getCentreY();

        juce::Path lowPath;
        lowPath.startNewSubPath (graph.getX() + 7.0f, midY - (band == 1 ? 0.0f : 13.0f));
        lowPath.cubicTo (split - 26.0f, midY - 13.0f, split - 12.0f, midY - 6.0f, split, midY);

        juce::Path highPath;
        highPath.startNewSubPath (split, midY);
        highPath.cubicTo (split + 18.0f, midY + (band == 0 ? 0.0f : -7.0f),
                          graph.getRight() - 28.0f, midY + (band == 0 ? 0.0f : -13.0f),
                          graph.getRight() - 7.0f, midY + (band == 0 ? 0.0f : -13.0f));

        g.setColour (band == 1 ? Palette::panelEdge : Palette::green);
        g.strokePath (lowPath, juce::PathStrokeType (2.0f));
        g.setColour (band == 0 ? Palette::panelEdge : Palette::gold);
        g.strokePath (highPath, juce::PathStrokeType (2.0f));

        g.setColour (Palette::orange);
        g.drawLine (split, graph.getY() + 6.0f, split, graph.getBottom() - 6.0f, 1.2f);
        g.fillEllipse (split - 3.0f, midY - 3.0f, 6.0f, 6.0f);

        g.setColour (Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
        g.drawText (juce::String ((int) freq) + " Hz", graph.toNearestInt().reduced (6, 4), juce::Justification::bottomRight);
        g.drawText (band == 0 ? "LOW" : band == 1 ? "HIGH" : "BOTH", graph.toNearestInt().reduced (6, 4), juce::Justification::topRight);
    }

    void drawHarmonics (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const float bias = apvts.getRawParameterValue (ParamID::harmonicBias)->load() * 0.01f;

        g.setColour (Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText ("HARM", area.removeFromTop (12.0f).toNearestInt(), juce::Justification::centred);

        auto meter = area.reduced (4.0f, 3.0f);
        g.setColour (juce::Colour (0xff05070b));
        g.fillRoundedRectangle (meter, 5.0f);

        const float centreX = meter.getCentreX();
        g.setColour (Palette::panelEdge);
        g.drawLine (centreX, meter.getY() + 4.0f, centreX, meter.getBottom() - 4.0f, 1.0f);

        const float x = juce::jmap (bias, meter.getX() + 8.0f, meter.getRight() - 8.0f);
        juce::ColourGradient grad (Palette::green, meter.getX(), meter.getCentreY(),
                                   Palette::gold, meter.getRight(), meter.getCentreY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (meter.withWidth (juce::jmax (1.0f, x - meter.getX())), 5.0f);
        g.setColour (Palette::goldBright);
        g.fillEllipse (x - 4.0f, meter.getCentreY() - 4.0f, 8.0f, 8.0f);

        g.setColour (Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
        g.drawText ("EVEN", meter.toNearestInt().reduced (5, 3), juce::Justification::bottomLeft);
        g.drawText ("ODD", meter.toNearestInt().reduced (5, 3), juce::Justification::bottomRight);
    }

    void drawWidth (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const float width = apvts.getRawParameterValue (ParamID::width)->load() * 0.01f;
        const float link = apvts.getRawParameterValue (ParamID::stereoLink)->load() * 0.01f;

        g.setColour (Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText ("WIDTH / LINK", area.removeFromTop (12.0f).toNearestInt(), juce::Justification::centred);

        auto scope = area.reduced (0.0f, 3.0f);
        g.setColour (juce::Colour (0xff05070b));
        g.fillRoundedRectangle (scope, 5.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (scope, 5.0f, 1.0f);

        const auto centre = scope.getCentre();
        const float w = juce::jlimit (12.0f, scope.getWidth() * 0.46f, scope.getWidth() * 0.18f * width);
        const float h = juce::jmap (link, 0.0f, 1.0f, scope.getHeight() * 0.34f, scope.getHeight() * 0.12f);

        g.setColour (Palette::green.withAlpha (0.18f));
        g.fillEllipse (centre.x - w, centre.y - h, w * 2.0f, h * 2.0f);
        g.setColour (Palette::greenBright);
        g.drawEllipse (centre.x - w, centre.y - h, w * 2.0f, h * 2.0f, 1.5f);

        g.setColour (Palette::gold);
        g.drawLine (centre.x - w, centre.y, centre.x + w, centre.y, 1.5f);

        g.setColour (Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
        g.drawText ("W " + juce::String ((int) std::round (width * 100.0f)) + "%",
                    scope.toNearestInt().reduced (6, 3), juce::Justification::bottomLeft);
        g.drawText ("L " + juce::String ((int) std::round (link * 100.0f)) + "%",
                    scope.toNearestInt().reduced (6, 3), juce::Justification::bottomRight);
    }

    juce::AudioProcessorValueTreeState& apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlStripDisplay)
};
