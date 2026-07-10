#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Parameters.h"
#include "PluginLookAndFeel.h"

//==============================================================================
//  Real-time animated Harmonic Analyzer component.
//  Displays 6 frequency bands (Fundamental + 5 harmonics) as animated bars.
//  Always shows the spectrum view — no view mode switching.
//  Reflects the current saturation mode and harmonic bias setting.
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

        const auto bounds = getLocalBounds().toFloat();
        
        // Draw background
        g.setColour (Palette::background.withAlpha (0.9f));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (bounds, 8.0f, 1.5f);

        drawHarmonicAnalyzer (g, bounds, mode, harmonicBias);
    }

private:
    void timerCallback() override
    {
        repaint();
    }

    void drawHarmonicAnalyzer (juce::Graphics& g, juce::Rectangle<float> area,
                                SatMode mode, float harmonicBias)
    {
        g.saveState();
        g.reduceClipRegion (area.toNearestInt());

        // Draw grid lines
        g.setColour (Palette::textDim.withAlpha (0.15f));
        for (float db = -12.0f; db >= -60.0f; db -= 12.0f)
        {
            float y = area.getY() + area.getHeight() * (db / -60.0f);
            g.drawHorizontalLine ((int)y, area.getX(), area.getRight());
        }

        const float numBars = 6;
        const float barPadding = 16.0f;
        const float totalWidth = area.getWidth() - (barPadding * 2.0f);
        const float barWidth = (totalWidth / numBars) - 8.0f;
        
        const juce::String labels[] = { "FUND", "2ND", "3RD", "4TH", "5TH", "6TH" };
        
        // Estimate harmonic response based on mode, input level, AND harmonic bias
        float hFactor[6] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        const float d = currentDrive;
        const float evenW = 1.0f - harmonicBias;  // even harmonic weight
        const float oddW  = harmonicBias;          // odd harmonic weight

        hFactor[0] = currentInputLevel; // Fundamental always tracks input

        if (hFactor[0] > 0.01f)
        {
            // Base harmonic profiles per mode (even harmonics: 2nd, 4th; odd: 3rd, 5th)
            float h2Base = 0.0f, h3Base = 0.0f, h4Base = 0.0f, h5Base = 0.0f;

            switch (mode)
            {
                case SatMode::Tube:
                    h2Base = 0.55f; h3Base = 0.25f; h4Base = 0.20f; h5Base = 0.10f;
                    break;
                case SatMode::ClassA:
                    h2Base = 0.40f; h3Base = 0.35f; h4Base = 0.15f; h5Base = 0.15f;
                    break;
                case SatMode::Tape:
                    h2Base = 0.15f; h3Base = 0.45f; h4Base = 0.08f; h5Base = 0.20f;
                    break;
                case SatMode::Transformer:
                    h2Base = 0.20f; h3Base = 0.50f; h4Base = 0.30f; h5Base = 0.35f;
                    break;
            }

            // Scale even harmonics (2nd, 4th) by evenW, odd (3rd, 5th) by oddW
            hFactor[1] = hFactor[0] * d * h2Base * (0.3f + evenW * 1.4f); // 2nd (even)
            hFactor[2] = hFactor[0] * d * h3Base * (0.3f + oddW * 1.4f);  // 3rd (odd)
            hFactor[3] = hFactor[0] * d * h4Base * (0.3f + evenW * 1.4f); // 4th (even)
            hFactor[4] = hFactor[0] * d * h5Base * (0.3f + oddW * 1.4f);  // 5th (odd)
            hFactor[5] = hFactor[2] * 0.25f; // 6th always small
        }

        // Render bars
        for (int i = 0; i < 6; ++i)
        {
            float db = juce::Decibels::gainToDecibels (hFactor[i]);
            if (db < -60.0f) db = -60.0f;
            
            // Normalize level: 0 at -60dB, 1 at 0dB
            float normalized = (db + 60.0f) / 60.0f;
            
            // Add subtle animated wiggles to look dynamic
            if (currentInputLevel > 0.005f)
                normalized += rand.nextFloat() * 0.03f * currentInputLevel;
            normalized = juce::jlimit (0.01f, 1.0f, normalized);

            float x = area.getX() + barPadding + i * (barWidth + 8.0f) + 4.0f;
            float y = area.getBottom() - 25.0f - (normalized * (area.getHeight() - 40.0f));
            float w = barWidth;
            float h = normalized * (area.getHeight() - 40.0f);

            // Draw bar gradient (Green to Orange/Red)
            juce::ColourGradient barGrad (Palette::green, x, area.getBottom() - 25.0f,
                                         Palette::orange, x, area.getY() + 15.0f, false);
            barGrad.addColour (0.8f, Palette::gold);
            
            g.setGradientFill (barGrad);
            g.fillRoundedRectangle (x, y, w, h, 3.0f);

            // Draw label
            g.setColour (Palette::textGrey);
            g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
            g.drawText (labels[i], (int)(x - 4.0f), (int)(area.getBottom() - 22.0f), (int)(w + 8.0f), 15, juce::Justification::centred);
        }

        g.restoreState();
    }

    juce::AudioProcessorValueTreeState& apvts;
    float currentInputLevel { 0.0f };
    float currentDrive { 0.0f };
    juce::Random rand;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Visualizer)
};
