#pragma once

#include <juce_dsp/juce_dsp.h>

//==============================================================================
//  A single-knob "tilt" tone control.
//    tone < 50  -> darker (high frequencies attenuated, lows lifted)
//    tone = 50  -> flat
//    tone > 50  -> brighter (high frequencies lifted, lows attenuated)
//  Implemented as complementary low/high shelves around a fixed pivot.
//==============================================================================
class ToneFilter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        lowShelf .prepare (spec);
        highShelf.prepare (spec);
        bassShelf.prepare (spec);
        update (50.0f, 0.0f);
    }

    void reset()
    {
        lowShelf.reset();
        highShelf.reset();
        bassShelf.reset();
    }

    // tone : 0..100, bassDb : -12..12
    void update (float tone, float bassDb) noexcept
    {
        const float t = juce::jlimit (0.0f, 100.0f, tone);
        const float tilt = (t - 50.0f) / 50.0f;        // -1 .. +1
        const float maxGainDb = 9.0f;

        const float highGainDb =  tilt * maxGainDb;
        
        // Limit low-shelf attenuation to -2.0 dB when tilting bright, preserving low end.
        float lowGainDb  = -tilt * maxGainDb;
        if (lowGainDb < -2.0f)
            lowGainDb = -2.0f;

        *lowShelf.state  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (
            sampleRate, pivotHz, 0.5f, juce::Decibels::decibelsToGain (lowGainDb));
        *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sampleRate, pivotHz, 0.5f, juce::Decibels::decibelsToGain (highGainDb));

        // Bass shelf filter at 150 Hz
        *bassShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            sampleRate, 150.0f, 0.5f, juce::Decibels::decibelsToGain (bassDb));
    }

    template <typename ProcessContext>
    void process (const ProcessContext& ctx) noexcept
    {
        lowShelf.process (ctx);
        highShelf.process (ctx);
        bassShelf.process (ctx);
    }

private:
    using Filter = juce::dsp::ProcessorDuplicator<
                       juce::dsp::IIR::Filter<float>,
                       juce::dsp::IIR::Coefficients<float>>;

    Filter lowShelf, highShelf, bassShelf;
    double sampleRate { 48000.0 };
    static constexpr float pivotHz = 700.0f;
};
