#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
//  Central place for every parameter ID, name and choice list.
//  Keeping them here means the processor and editor never disagree on a string.
//==============================================================================
namespace ParamID
{
    static const juce::String drive      { "drive" };
    static const juce::String tone       { "tone" };
    static const juce::String bass       { "bass" };
    static const juce::String output     { "output" };
    static const juce::String mix        { "mix" };
    static const juce::String mode       { "mode" };
    static const juce::String oversample { "oversample" };
    static const juce::String active     { "active" };
    static const juce::String bypass     { "bypass" };
    static const juce::String hiss       { "hiss" };   // tape hiss amount
    static const juce::String autoGain   { "autoGain" };
    static const juce::String input      { "input" };
    static const juce::String punch      { "punch" };
    static const juce::String delta      { "delta" };
    static const juce::String character  { "character" };
    static const juce::String harmonicBias { "harmonicBias" };
    static const juce::String crossoverFreq { "crossoverFreq" };
    static const juce::String bandSelect   { "bandSelect" };
    static const juce::String clipMode     { "clipMode" };
    static const juce::String stereoLink   { "stereoLink" };
    static const juce::String width        { "width" };
}

namespace ParamChoices
{
    // Order here defines the stored index — never reorder, only append.
    inline juce::StringArray modes()       { return { "Tube", "Class A", "Tape", "Transformer" }; }
    inline juce::StringArray oversample()  { return { "Off", "2x", "4x", "8x" }; }

    inline juce::StringArray bandSelect()  { return { "Low", "High", "Both" }; }
    inline juce::StringArray clipModes()   { return { "Soft Clip", "Hard Clip" }; }
}

enum class SatMode { Tube = 0, ClassA = 1, Tape = 2, Transformer = 3 };
enum class OverSampleMode { Off = 0, x2 = 1, x4 = 2, x8 = 3 };

//==============================================================================
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (float v) { return String (v, 0) + "%"; };

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::drive, 1 }, "Drive",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::tone, 1 }, "Tone",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            if (v < 49.0f) return String ("Dark ")  + String (50.0f - v, 0);
            if (v > 51.0f) return String ("Bright ") + String (v - 50.0f, 0);
            return String ("Flat");
        })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::bass, 1 }, "Bass",
        NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::output, 1 }, "Output",
        NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::mix, 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::mode, 1 }, "Mode", ParamChoices::modes(), 0));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::oversample, 1 }, "Oversample", ParamChoices::oversample(), 1));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::active, 1 }, "Active", true));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::bypass, 1 }, "Bypass", false));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::hiss, 1 }, "Tape Hiss",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 25.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::autoGain, 1 }, "Auto Gain", true));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::input, 1 }, "Input",
        NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::punch, 1 }, "Punch",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::character, 1 }, "Character",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::delta, 1 }, "Delta", false));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::harmonicBias, 1 }, "Harmonic Bias",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::crossoverFreq, 1 }, "Crossover Frequency",
        NormalisableRange<float> (20.0f, 250.0f, 1.0f, 0.45f), 120.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 0) + " Hz"; })));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::bandSelect, 1 }, "Band Select", ParamChoices::bandSelect(), 2));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::clipMode, 1 }, "Clipping Mode", ParamChoices::clipModes(), 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::stereoLink, 1 }, "Stereo Link",
        NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::width, 1 }, "Width",
        NormalisableRange<float> (0.0f, 200.0f, 0.01f), 100.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([pct] (float v, int) { return pct (v); })));

    return layout;
}
