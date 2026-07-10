#include "PluginEditor.h"
#include "BinaryData.h"
#include <vector>
#include <utility>

//==============================================================================
namespace
{
    constexpr int kWidth  = 1000;
    constexpr int kHeight = 600;
}

//==============================================================================
WapDemSaturationEditor::WapDemSaturationEditor (WapDemSaturationProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      harmonicsVisualizer (p.apvts)
{
    setLookAndFeel (&lnf);

    // ---- Header components ----

    presetName.setJustificationType (juce::Justification::centred);
    presetName.setColour (juce::Label::textColourId, Palette::textBright);
    presetName.setColour (juce::Label::backgroundColourId, Palette::panel);
    presetName.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::plain)));
    addAndMakeVisible (presetName);
    refreshPresetLabel();

    auto styleHeaderBtn = [this] (juce::TextButton& b, const juce::Colour& textColour)
    {
        b.setColour (juce::TextButton::buttonColourId, Palette::panel);
        b.setColour (juce::TextButton::textColourOffId, textColour);
        addAndMakeVisible (b);
    };

    styleHeaderBtn (prevPreset, Palette::textBright);
    styleHeaderBtn (nextPreset, Palette::textBright);
    styleHeaderBtn (savePreset, Palette::textGrey);
    styleHeaderBtn (undoBtn,   Palette::textGrey);
    styleHeaderBtn (redoBtn,   Palette::textGrey);
    styleHeaderBtn (bypassBtn, Palette::red);
    styleHeaderBtn (loadBtn,   Palette::textBright);

    prevPreset.onClick = [this]
    {
        int idx = proc.getCurrentProgram() - 1;
        if (idx < 0) idx = proc.getNumPrograms() - 1;
        proc.setCurrentProgram (idx);
        refreshPresetLabel();
    };

    nextPreset.onClick = [this]
    {
        int idx = (proc.getCurrentProgram() + 1) % proc.getNumPrograms();
        proc.setCurrentProgram (idx);
        refreshPresetLabel();
    };

    bypassBtn.onClick = [this]
    {
        if (auto* param = proc.apvts.getParameter (ParamID::bypass))
        {
            const bool bypassed = !param->getValue();
            param->setValueNotifyingHost (bypassed ? 1.0f : 0.0f);
            bypassBtn.setToggleState (bypassed, juce::dontSendNotification);
            bypassBtn.setColour (juce::TextButton::buttonColourId, bypassed ? Palette::red : Palette::panel);
        }
    };

    loadBtn.onClick = [this]
    {
        auto presetDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
            .getChildFile ("Tractone Audio")
            .getChildFile ("WapDem Saturation")
            .getChildFile ("Presets");
        presetDir.createDirectory();

        fileChooser = std::make_unique<juce::FileChooser> ("Load Preset", presetDir, "*.xml");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file != juce::File{})
                {
                    if (auto xml = juce::XmlDocument::parse (file))
                    {
                        if (xml->hasTagName (proc.apvts.state.getType()))
                        {
                            proc.setCurrentProgram (xml->getIntAttribute ("program", 0));
                            proc.apvts.replaceState (juce::ValueTree::fromXml (*xml));
                            refreshPresetLabel();
                        }
                    }
                }
            });
    };

    // ---- Left Panel controls ----
    buildKnob (inputKnob, inputLbl, "INPUT", "input");
    buildKnob (driveKnob, driveLbl, "DRIVE", "drive");
    buildKnob (punchKnob, punchLbl, "PUNCH", "punch");
    buildKnob (mixKnobL,  mixLblL,  "MIX",   "mixL");

    addAndMakeVisible (inMeter);
    inMeterLbl.setJustificationType (juce::Justification::centred);
    inMeterLbl.setColour (juce::Label::textColourId, Palette::textGrey);
    inMeterLbl.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    addAndMakeVisible (inMeterLbl);

    // ---- Center Panel controls ----
    addAndMakeVisible (harmonicsVisualizer);

    buildComboBox (characterBox, characterLbl, "CHARACTER", ParamChoices::modes());
    buildComboBox (modeBox,      modeLbl,      "MODE",        ParamChoices::clipModes());
    buildComboBox (oversampleBox, oversampleLbl, "OVERSAMPLING", ParamChoices::oversample());

    // ---- Right Panel controls ----
    buildKnob (mixKnob,    mixLbl,    "MIX",    "mix");
    buildKnob (outputKnob, outputLbl, "OUTPUT", "output");

    addAndMakeVisible (outMeter);
    outMeterLbl.setJustificationType (juce::Justification::centred);
    outMeterLbl.setColour (juce::Label::textColourId, Palette::textGrey);
    outMeterLbl.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    addAndMakeVisible (outMeterLbl);

    // ---- Bottom Bar controls ----
    autoGainBtn.setClickingTogglesState (true);
    autoGainBtn.setColour (juce::ToggleButton::textColourId, Palette::textGrey);
    addAndMakeVisible (autoGainBtn);

    deltaBtn.setClickingTogglesState (true);
    deltaBtn.setColour (juce::ToggleButton::textColourId, Palette::textGrey);
    addAndMakeVisible (deltaBtn);

    bandSelectLbl.setJustificationType (juce::Justification::centred);
    bandSelectLbl.setColour (juce::Label::textColourId, Palette::textDim);
    bandSelectLbl.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    addAndMakeVisible (bandSelectLbl);

    auto styleBandBtn = [this] (juce::TextButton& b, int idx)
    {
        addAndMakeVisible (b);
        b.onClick = [this, idx]
        {
            if (auto* param = proc.apvts.getParameter (ParamID::bandSelect))
                param->setValueNotifyingHost (param->convertTo0to1 ((float)idx));
            updateStereoLinkButtonColors (idx);
        };
    };
    styleBandBtn (bandLowBtn,  0);
    styleBandBtn (bandHighBtn, 1);
    styleBandBtn (bandBothBtn, 2);

    const int activeBand = (int) proc.apvts.getRawParameterValue (ParamID::bandSelect)->load();
    updateStereoLinkButtonColors (activeBand);

    buildKnob (crossoverFreqKnob, crossoverFreqLbl, "FREQ", "crossoverFreq");

    buildKnob (harmonicBiasKnob, harmonicBiasLbl, "HARMONICS", "harmonicBias");
    buildKnob (widthKnob, widthLbl, "WIDTH", "width");
    buildKnob (stereoLinkKnob, stereoLinkLbl, "LINK", "stereoLink");

    // ---- Attachments ----
    auto& s = proc.apvts;
    inputAtt      = std::make_unique<SliderAtt> (s, ParamID::input,      inputKnob);
    driveAtt      = std::make_unique<SliderAtt> (s, ParamID::drive,      driveKnob);
    punchAtt      = std::make_unique<SliderAtt> (s, ParamID::punch,      punchKnob);
    mixLAtt       = std::make_unique<SliderAtt> (s, ParamID::mix,        mixKnobL);
    mixAtt        = std::make_unique<SliderAtt> (s, ParamID::mix,        mixKnob);
    outputAtt     = std::make_unique<SliderAtt> (s, ParamID::output,     outputKnob);
    crossoverFreqAtt = std::make_unique<SliderAtt> (s, ParamID::crossoverFreq, crossoverFreqKnob);
    widthAtt      = std::make_unique<SliderAtt> (s, ParamID::width,      widthKnob);
    stereoLinkAtt = std::make_unique<SliderAtt> (s, ParamID::stereoLink, stereoLinkKnob);
    harmonicBiasAtt = std::make_unique<SliderAtt> (s, ParamID::harmonicBias, harmonicBiasKnob);

    characterAtt  = std::make_unique<ComboAtt>  (s, ParamID::mode,       characterBox);
    modeAtt       = std::make_unique<ComboAtt>  (s, ParamID::clipMode,   modeBox);
    oversampleAtt = std::make_unique<ComboAtt>  (s, ParamID::oversample, oversampleBox);

    autoGainAtt   = std::make_unique<ButtonAtt> (s, ParamID::autoGain,   autoGainBtn);
    deltaAtt      = std::make_unique<ButtonAtt> (s, ParamID::delta,      deltaBtn);

    // --- Process logo: strip black background to transparent ---
    {
        auto raw = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
        if (raw.isValid())
        {
            processedLogoImg = raw.convertedToFormat (juce::Image::ARGB);
            const int width = processedLogoImg.getWidth();
            const int height = processedLogoImg.getHeight();
            
            std::vector<std::vector<bool>> isBackground ((size_t)width, std::vector<bool> ((size_t)height, false));
            std::vector<std::pair<int, int>> queue;
            queue.reserve ((size_t)(width * height));
            
            juce::Image::BitmapData bmp (processedLogoImg, juce::Image::BitmapData::readWrite);
            
            auto isDark = [&bmp] (int x, int y) -> bool
            {
                auto c = bmp.getPixelColour (x, y);
                return ((int)c.getRed() + (int)c.getGreen() + (int)c.getBlue()) < 60;
            };
            
            // Seed the flood fill from all four outer boundaries
            for (int x = 0; x < width; ++x)
            {
                if (isDark (x, 0))
                {
                    isBackground[(size_t)x][0] = true;
                    queue.push_back ({ x, 0 });
                }
                if (isDark (x, height - 1))
                {
                    isBackground[(size_t)x][(size_t)(height - 1)] = true;
                    queue.push_back ({ x, height - 1 });
                }
            }
            for (int y = 0; y < height; ++y)
            {
                if (isDark (0, y))
                {
                    if (!isBackground[0][(size_t)y])
                    {
                        isBackground[0][(size_t)y] = true;
                        queue.push_back ({ 0, y });
                    }
                }
                if (isDark (width - 1, y))
                {
                    if (!isBackground[(size_t)(width - 1)][(size_t)y])
                    {
                        isBackground[(size_t)(width - 1)][(size_t)y] = true;
                        queue.push_back ({ width - 1, y });
                    }
                }
            }
            
            // Perform flood fill to find all contiguous dark outer background pixels
            size_t head = 0;
            const int dx[4] = { -1, 1, 0, 0 };
            const int dy[4] = { 0, 0, -1, 1 };
            
            while (head < queue.size())
            {
                auto curr = queue[head++];
                int cx = curr.first;
                int cy = curr.second;
                
                for (int i = 0; i < 4; ++i)
                {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                    {
                        if (!isBackground[(size_t)nx][(size_t)ny] && isDark (nx, ny))
                        {
                            isBackground[(size_t)nx][(size_t)ny] = true;
                            queue.push_back ({ nx, ny });
                        }
                    }
                }
            }
            
            // Make only the detected background pixels transparent
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (isBackground[(size_t)x][(size_t)y])
                    {
                        bmp.setPixelColour (x, y, juce::Colours::transparentBlack);
                    }
                }
            }
        }
    }

    setSize (kWidth, kHeight);
    startTimerHz (30);
}

WapDemSaturationEditor::~WapDemSaturationEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void WapDemSaturationEditor::buildKnob (juce::Slider& k, juce::Label& l, const juce::String& text, const juce::String& componentId)
{
    k.setComponentID (componentId);
    k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (k);

    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, Palette::textGrey);
    l.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    addAndMakeVisible (l);
}

void WapDemSaturationEditor::buildComboBox (juce::ComboBox& box, juce::Label& l, const juce::String& text, const juce::StringArray& choices)
{
    box.addItemList (choices, 1);
    addAndMakeVisible (box);

    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, Palette::textDim);
    l.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    addAndMakeVisible (l);
}

void WapDemSaturationEditor::refreshPresetLabel()
{
    presetName.setText (proc.getProgramName (proc.getCurrentProgram()), juce::dontSendNotification);
}

void WapDemSaturationEditor::updateStereoLinkButtonColors (int activeIndex)
{
    juce::TextButton* btns[] = { &bandLowBtn, &bandHighBtn, &bandBothBtn };
    for (int i = 0; i < 3; ++i)
    {
        btns[i]->setColour (juce::TextButton::buttonColourId, (i == activeIndex) ? Palette::green : Palette::panel);
        btns[i]->setColour (juce::TextButton::textColourOffId, (i == activeIndex) ? Palette::background : Palette::textGrey);
    }
}



//==============================================================================
void WapDemSaturationEditor::timerCallback()
{
    float inL = proc.getInputLevel (0);
    float inR = proc.getInputLevel (1);
    float outL = proc.getOutputLevel (0);
    float outR = proc.getOutputLevel (1);

    inMeter .setLevels (inL, inR);
    outMeter.setLevels (outL, outR);

    // Peak levels sent to visualizers
    float peakIn = juce::jmax (inL, inR);
    float driveScaled = proc.apvts.getRawParameterValue (ParamID::drive)->load() * 0.01f;
    harmonicsVisualizer.setLevels (peakIn, driveScaled);

    // Sync button highlight states in case presets were loaded
    const int activeBand = (int) proc.apvts.getRawParameterValue (ParamID::bandSelect)->load();
    updateStereoLinkButtonColors (activeBand);

    const bool bypassed = proc.apvts.getRawParameterValue (ParamID::bypass)->load() > 0.5f;
    bypassBtn.setColour (juce::TextButton::buttonColourId, bypassed ? Palette::red : Palette::panel);
}

void WapDemSaturationEditor::paint (juce::Graphics& g)
{
    // Draw premium slate dark background
    g.fillAll (Palette::background);

    // Header Panel container
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (15.0f, 12.0f, 970.0f, 48.0f, 6.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (15.0f, 12.0f, 970.0f, 48.0f, 6.0f, 1.2f);

    // Left controls panel
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (15.0f, 75.0f, 280.0f, 435.0f, 8.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (15.0f, 75.0f, 280.0f, 435.0f, 8.0f, 1.2f);

    // Center panel
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (310.0f, 75.0f, 410.0f, 435.0f, 8.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (310.0f, 75.0f, 410.0f, 435.0f, 8.0f, 1.2f);

    // Right panel
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (735.0f, 75.0f, 250.0f, 435.0f, 8.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (735.0f, 75.0f, 250.0f, 435.0f, 8.0f, 1.2f);

    // Bottom panel
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (15.0f, 520.0f, 970.0f, 68.0f, 8.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (15.0f, 520.0f, 970.0f, 68.0f, 8.0f, 1.2f);

    // --- Header Text & Logo ---
    if (processedLogoImg.isValid())
    {
        // Draw gold crown T logo with transparent background
        g.drawImageWithin (processedLogoImg, 18, 8, 44, 44, juce::Justification::centred, false);
    }

    g.setColour (Palette::textBright);
    g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    g.drawText ("TRACTONE AUDIO", 68, 20, 200, 28, juce::Justification::left);

    // --- Center Title / Subtitle inside Center Panel ---
    g.setColour (Palette::textBright);
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("WAPDEM SATURATION", 310, 95, 410, 25, juce::Justification::centred);
    
    g.setColour (Palette::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("HARMONIC DRIVE UNIT", 310, 125, 410, 12, juce::Justification::centred);

    // Draw a subtle line separator under subtitle
    g.setColour (Palette::panelEdge);
    g.drawHorizontalLine (145, 330.0f, 700.0f);

    // Version info
    g.setColour (Palette::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("v2.3.0", 915, 545, 55, 20, juce::Justification::right);
}

//==============================================================================
void WapDemSaturationEditor::resized()
{
    // ---- Header components ----
    prevPreset.setBounds (340, 22, 25, 26);
    presetName.setBounds (370, 22, 140, 26);
    nextPreset.setBounds (515, 22, 25, 26);
    savePreset.setBounds (555, 22, 50, 26);
    undoBtn   .setBounds (615, 22, 50, 26);
    redoBtn   .setBounds (675, 22, 50, 26);
    
    bypassBtn .setBounds (830, 22, 70, 26);
    loadBtn   .setBounds (910, 22, 60, 26);

    // ---- Left Panel controls ----
    inMeter   .setBounds (25, 120, 28, 360);
    inMeterLbl.setBounds (25, 95, 28, 20);

    inputKnob.setBounds (70, 240, 80, 80);
    inputLbl .setBounds (70, 325, 80, 15);

    driveKnob.setBounds (165, 95, 105, 105);
    driveLbl .setBounds (165, 202, 105, 15);

    punchKnob.setBounds (175, 230, 85, 85);
    punchLbl .setBounds (175, 317, 85, 15);

    mixKnobL .setBounds (175, 345, 85, 85);
    mixLblL  .setBounds (175, 432, 85, 15);

    // ---- Center Panel controls ----
    harmonicsVisualizer.setBounds (330, 160, 235, 325);

    characterLbl.setBounds (580, 175, 120, 15);
    characterBox.setBounds (580, 195, 120, 28);

    modeLbl.setBounds (580, 265, 120, 15);
    modeBox.setBounds (580, 285, 120, 28);

    oversampleLbl.setBounds (580, 355, 120, 15);
    oversampleBox.setBounds (580, 375, 120, 28);

    // ---- Right Panel controls ----
    mixKnob   .setBounds (760, 110, 110, 110);
    mixLbl    .setBounds (760, 222, 110, 15);

    outputKnob.setBounds (760, 270, 110, 110);
    outputLbl .setBounds (760, 382, 110, 15);

    outMeter   .setBounds (895, 120, 28, 360);
    outMeterLbl.setBounds (895, 95, 28, 20);

    // ---- Bottom Bar controls ----
    autoGainBtn.setBounds (30, 539, 100, 30);
    deltaBtn   .setBounds (140, 539, 80, 30);

    bandSelectLbl.setBounds (240, 523, 160, 15);
    bandLowBtn   .setBounds (240, 541, 53, 26);
    bandHighBtn  .setBounds (293, 541, 53, 26);
    bandBothBtn  .setBounds (346, 541, 54, 26);

    crossoverFreqKnob.setBounds (465, 523, 54, 54);
    crossoverFreqLbl .setBounds (450, 574, 84, 12);

    harmonicBiasKnob.setBounds (565, 523, 54, 54);
    harmonicBiasLbl .setBounds (548, 574, 88, 12);

    widthKnob.setBounds (745, 523, 54, 54);
    widthLbl .setBounds (730, 574, 84, 12);

    stereoLinkKnob.setBounds (825, 523, 54, 54);
    stereoLinkLbl .setBounds (810, 574, 84, 12);
}
