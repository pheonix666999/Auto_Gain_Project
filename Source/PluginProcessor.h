#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "dsp/Saturation.h"
#include "dsp/ToneFilter.h"
#include "dsp/LevelMeter.h"

//==============================================================================
class WapDemSaturationProcessor : public juce::AudioProcessor
{
public:
    WapDemSaturationProcessor();
    ~WapDemSaturationProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==========================================================================
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    // Meter access for the editor (0..1, smoothed on read).
    float getInputLevel  (int ch) noexcept { return inMeter [juce::jlimit (0, 1, ch)].readLevel(); }
    float getOutputLevel (int ch) noexcept { return outMeter[juce::jlimit (0, 1, ch)].readLevel(); }

    //==========================================================================
    // Lightweight preset handling (factory presets stored as APVTS states).
    void loadFactoryPreset (int index);

private:
    void updateParameters();
    OverSampleMode currentOversampleMode() const noexcept;

    //==========================================================================
    Saturation saturation[2];          // per channel
    ToneFilter tone;
    juce::dsp::LinkwitzRileyFilter<float> crossover[2];

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler2x;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler4x;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler8x;
    OverSampleMode activeOversample { OverSampleMode::x2 };

    juce::dsp::ProcessSpec spec {};

    juce::SmoothedValue<float> inputSm, driveSm, toneSm, bassSm, characterSm, mixSm, outputSm, hissSm, punchSm, crossoverFreqSm, stereoLinkSm, widthSm, harmonicBiasSm;
    juce::SmoothedValue<float> autoGainCompSm;
    float punchState[2] { 0.0f, 0.0f };

    juce::Random hissRng;

    LevelMeter inMeter[2], outMeter[2];

    int currentProgram { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WapDemSaturationProcessor)
};
