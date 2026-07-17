#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "gui/PluginLookAndFeel.h"
#include "gui/VerticalMeter.h"
#include "gui/Visualizer.h"

//==============================================================================
class WapDemSaturationEditor : public juce::AudioProcessorEditor,
                               private juce::Timer
{
public:
    explicit WapDemSaturationEditor (WapDemSaturationProcessor&);
    ~WapDemSaturationEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildKnob (juce::Slider&, juce::Label&, const juce::String& text, const juce::String& componentId);
    void buildComboBox (juce::ComboBox&, juce::Label&, const juce::String& text, const juce::StringArray& choices);
    void refreshPresetLabel();
    
    void updateStereoLinkButtonColors (int activeIndex);

    WapDemSaturationProcessor& proc;
    PluginLookAndFeel lnf;
    juce::Image processedLogoImg;  // logo with black background stripped to transparent

    // --- Header components ---
    juce::Label      presetName;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "Save" };
    juce::TextButton undoBtn { "Undo" }, redoBtn { "Redo" };
    juce::TextButton bypassBtn { "BYPASS" };
    juce::TextButton loadBtn { "Load" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    // --- Left Panel controls ---
    juce::Slider inputKnob, driveKnob, punchKnob;
    juce::Label  inputLbl, driveLbl, punchLbl;
    VerticalMeter inMeter;
    juce::Label   inMeterLbl { {}, "IN" };

    // --- Center Panel controls ---
    Visualizer harmonicsVisualizer;
    juce::ComboBox characterBox, modeBox, oversampleBox;
    juce::Label    characterLbl { {}, "CHARACTER" }, modeLbl { {}, "MODE" }, oversampleLbl { {}, "OVERSAMPLING" };

    // --- Right Panel controls ---
    juce::Slider mixKnob, outputKnob;
    juce::Label  mixLbl, outputLbl;
    VerticalMeter outMeter;
    juce::Label   outMeterLbl { {}, "OUT" };

    // --- Bottom Bar controls ---
    juce::ToggleButton autoGainBtn { "AUTO GAIN" };
    juce::ToggleButton deltaBtn { "DELTA" };
    
    juce::Label bandSelectLbl { {}, "BAND SELECT" };
    juce::TextButton bandLowBtn { "LOW" }, bandHighBtn { "HIGH" }, bandBothBtn { "BOTH" };

    juce::Slider crossoverFreqKnob;
    juce::Label  crossoverFreqLbl { {}, "FREQ" };

    juce::Slider harmonicBiasKnob;
    juce::Label  harmonicBiasLbl { {}, "HARMONICS" };

    juce::Slider widthKnob, stereoLinkKnob;
    juce::Label  widthLbl { {}, "WIDTH" }, stereoLinkLbl { {}, "LINK" };

    // --- Parameter attachments ---
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> inputAtt, driveAtt, punchAtt, mixAtt, outputAtt, crossoverFreqAtt, widthAtt, stereoLinkAtt, harmonicBiasAtt;
    std::unique_ptr<ComboAtt>  characterAtt, modeAtt, oversampleAtt;
    std::unique_ptr<ButtonAtt> autoGainAtt, deltaAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WapDemSaturationEditor)
};
