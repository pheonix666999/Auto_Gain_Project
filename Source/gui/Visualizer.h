#pragma once

#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Parameters.h"
#include "PluginLookAndFeel.h"

//==============================================================================
//  Compact parallel saturation core display.
//  Shows the dry/wet split, active saturation type, mix amount, punch contour,
//  and low-end preservation state without occupying the whole interface.
//==============================================================================
class Visualizer : public juce::Component, private juce::Timer
{
public:
    Visualizer (juce::AudioProcessorValueTreeState& state)
        : apvts (state)
    {
        startTimerHz (30); // 30 FPS update rate
    }

    ~Visualizer() override
    {
        stopTimer();
    }

    void setLevels (float inputLevel, float drive01)
    {
        currentInputLevel = inputLevel;
        currentDrive = drive01;
    }

    void paint (juce::Graphics& g) override
    {
        const auto mode = (SatMode) (int) apvts.getRawParameterValue (ParamID::mode)->load();
        const float harmonicBias = apvts.getRawParameterValue (ParamID::harmonicBias)->load() * 0.01f;
        const float mix = apvts.getRawParameterValue (ParamID::mix)->load() * 0.01f;
        const float punch = apvts.getRawParameterValue (ParamID::punch)->load() * 0.01f;
        const int band = (int) apvts.getRawParameterValue (ParamID::bandSelect)->load();
        const int clip = (int) apvts.getRawParameterValue (ParamID::clipMode)->load();

        const auto bounds = getLocalBounds().toFloat();

        juce::ColourGradient bg (juce::Colour (0xff080b12), bounds.getTopLeft(),
                                 juce::Colour (0xff15110a), bounds.getBottomRight(), false);
        bg.addColour (0.55, juce::Colour (0xff101724));
        g.setGradientFill (bg);
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (Palette::gold.withAlpha (0.35f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.2f);

        drawCoreDisplay (g, bounds.reduced (12.0f), mode, harmonicBias, mix, punch, band, clip);
    }

private:
    void timerCallback() override
    {
        repaint();
    }

    static const char* modeName (SatMode mode) noexcept
    {
        switch (mode)
        {
            case SatMode::Tube:        return "TUBE";
            case SatMode::ClassA:      return "CLASS A";
            case SatMode::Tape:        return "TAPE";
            case SatMode::Transformer: return "XFMR";
        }

        return "SAT";
    }

    static const char* bandName (int band) noexcept
    {
        if (band == 0) return "LOW";
        if (band == 1) return "HIGH";
        return "BOTH";
    }

    void drawPill (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                   juce::Colour colour, bool active)
    {
        g.setColour (active ? colour.withAlpha (0.24f) : juce::Colour (0xff0b0f17));
        g.fillRoundedRectangle (area, 5.0f);
        g.setColour (active ? colour : Palette::panelEdge);
        g.drawRoundedRectangle (area, 5.0f, 1.0f);
        g.setColour (active ? Palette::textBright : Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (text, area.toNearestInt(), juce::Justification::centred);
    }

    void drawCoreDisplay (juce::Graphics& g, juce::Rectangle<float> area, SatMode mode,
                          float harmonicBias, float mix, float punch, int band, int clip)
    {
        g.setColour (Palette::textBright);
        g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        g.drawText ("PARALLEL SATURATION CORE", area.removeFromTop (18.0f).toNearestInt(), juce::Justification::left);

        auto chipRow = area.removeFromTop (24.0f);
        drawPill (g, chipRow.removeFromLeft (65.0f), modeName (mode), Palette::gold, true);
        chipRow.removeFromLeft (7.0f);
        drawPill (g, chipRow.removeFromLeft (74.0f), clip == 0 ? "SOFT" : "HARD", Palette::orange, true);
        chipRow.removeFromLeft (7.0f);
        drawPill (g, chipRow.removeFromLeft (70.0f), bandName (band), Palette::green, true);

        area.removeFromTop (4.0f);
        auto lanes = area.removeFromTop (72.0f);
        auto dryLane = lanes.removeFromTop (28.0f);
        lanes.removeFromTop (8.0f);
        auto wetLane = lanes.removeFromTop (28.0f);

        drawLane (g, dryLane, "DRY", 1.0f - mix, Palette::textGrey);
        drawLane (g, wetLane, "WET", mix, Palette::gold);

        area.removeFromTop (6.0f);
        auto lower = area;
        auto orbit = lower.removeFromLeft (92.0f);
        lower.removeFromLeft (12.0f);
        auto curve = lower;

        drawHarmonicOrbit (g, orbit, harmonicBias);
        drawCurve (g, curve, currentDrive, punch);
    }

    void drawLane (juce::Graphics& g, juce::Rectangle<float> lane, const juce::String& label,
                   float amount, juce::Colour colour)
    {
        const auto textArea = lane.removeFromLeft (34.0f);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.setColour (Palette::textGrey);
        g.drawText (label, textArea.toNearestInt(), juce::Justification::centredLeft);

        const float centreY = lane.getCentreY();
        g.setColour (Palette::panelEdge);
        g.drawLine (lane.getX(), centreY, lane.getRight(), centreY, 2.0f);

        const auto active = lane.withWidth (lane.getWidth() * juce::jlimit (0.0f, 1.0f, amount));
        g.setColour (colour);
        g.drawLine (active.getX(), centreY, active.getRight(), centreY, 3.0f);

        for (int i = 0; i < 4; ++i)
        {
            const float x = lane.getX() + lane.getWidth() * (float) i / 3.0f;
            const bool lit = (float) i / 3.0f <= amount;
            g.setColour (lit ? colour : Palette::panelEdge);
            g.fillEllipse (x - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
        }
    }

    void drawHarmonicOrbit (juce::Graphics& g, juce::Rectangle<float> area, float harmonicBias)
    {
        const auto centre = area.getCentre();
        const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.38f;

        g.setColour (Palette::panelEdge);
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 1.0f);

        const int count = 8;
        for (int i = 0; i < count; ++i)
        {
            const float phase = (float) i / (float) count * juce::MathConstants<float>::twoPi
                              + harmonicBias * 0.7f;
            const float level = currentInputLevel * (0.3f + currentDrive * 0.7f);
            const float dot = 3.0f + level * (i % 2 == 0 ? 7.0f : 4.5f);
            const float x = centre.x + std::cos (phase) * radius;
            const float y = centre.y + std::sin (phase) * radius;

            g.setColour ((i % 2 == 0 ? Palette::gold : Palette::green).withAlpha (0.45f + currentDrive * 0.45f));
            g.fillEllipse (x - dot * 0.5f, y - dot * 0.5f, dot, dot);
        }

        g.setColour (Palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText ("HARM", area.removeFromBottom (14.0f).toNearestInt(), juce::Justification::centred);
    }

    void drawCurve (juce::Graphics& g, juce::Rectangle<float> area, float drive, float punch)
    {
        g.setColour (juce::Colour (0xff070a10));
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (area, 6.0f, 1.0f);

        auto plot = area.reduced (12.0f, 10.0f);
        g.setColour (Palette::textDim.withAlpha (0.18f));
        g.drawLine (plot.getX(), plot.getCentreY(), plot.getRight(), plot.getCentreY(), 1.0f);
        g.drawLine (plot.getCentreX(), plot.getY(), plot.getCentreX(), plot.getBottom(), 1.0f);

        juce::Path curvePath;
        const int points = 64;
        const float gain = 1.0f + drive * drive * 9.0f;
        for (int i = 0; i < points; ++i)
        {
            const float t = (float) i / (float) (points - 1);
            const float x = t * 2.0f - 1.0f;
            const float y = std::tanh (x * gain);
            const float px = plot.getX() + t * plot.getWidth();
            const float py = plot.getCentreY() - y * plot.getHeight() * 0.42f;

            if (i == 0) curvePath.startNewSubPath (px, py);
            else        curvePath.lineTo (px, py);
        }

        g.setColour (Palette::gold.withAlpha (0.25f));
        g.strokePath (curvePath, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        g.setColour (Palette::goldBright);
        g.strokePath (curvePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        g.setColour (Palette::textGrey);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText (juce::String ("DRIVE ") + juce::String (juce::roundToInt (drive * 100.0f)) + "%", area.toNearestInt().reduced (8, 4),
                    juce::Justification::topLeft);
        g.drawText (juce::String ("PUNCH ") + juce::String (juce::roundToInt (punch * 100.0f)) + "%", area.toNearestInt().reduced (8, 4),
                    juce::Justification::bottomRight);
    }

    juce::AudioProcessorValueTreeState& apvts;
    float currentInputLevel { 0.0f };
    float currentDrive { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Visualizer)
};
