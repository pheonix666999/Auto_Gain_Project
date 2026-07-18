#include "PluginEditor.h"
#include "BinaryData.h"
#include <vector>
#include <utility>

//==============================================================================
namespace
{
    constexpr int kWidth  = 860;
    constexpr int kHeight = 500;
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

    savePreset.onClick = [this]
    {
        auto presetDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
            .getChildFile ("Tractone Audio")
            .getChildFile ("WapDem Saturation")
            .getChildFile ("Presets");
        presetDir.createDirectory();

        fileChooser = std::make_unique<juce::FileChooser> ("Save Preset", presetDir.getChildFile ("WapDem Preset.xml"), "*.xml");
        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File{})
                    return;

                if (! file.hasFileExtension ("xml"))
                    file = file.withFileExtension ("xml");

                if (auto state = proc.apvts.copyState(); state.isValid())
                {
                    std::unique_ptr<juce::XmlElement> xml (state.createXml());
                    xml->setAttribute ("program", proc.getCurrentProgram());
                    file.replaceWithText (xml->toString());
                    refreshPresetLabel();
                }
            });
    };

    undoBtn.onClick = [this] { proc.undoManager.undo(); };
    redoBtn.onClick = [this] { proc.undoManager.redo(); };

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

    addAndMakeVisible (inMeter);
    inMeterLbl.setJustificationType (juce::Justification::centred);
    inMeterLbl.setColour (juce::Label::textColourId, Palette::textGrey);
    inMeterLbl.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    addAndMakeVisible (inMeterLbl);

    // ---- Center Panel controls ----
    addAndMakeVisible (harmonicsVisualizer);

    buildComboBox (characterBox, characterLbl, "SAT TYPE", ParamChoices::modes());
    buildComboBox (modeBox,      modeLbl,      "CLIP MODE", ParamChoices::clipModes());
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
    juce::ColourGradient bg (juce::Colour (0xff070910), 0.0f, 0.0f,
                             juce::Colour (0xff16100a), (float) kWidth, (float) kHeight, false);
    bg.addColour (0.38, juce::Colour (0xff0b121d));
    bg.addColour (0.72, juce::Colour (0xff10151f));
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (Palette::gold.withAlpha (0.05f));
    for (int x = -40; x < kWidth; x += 34)
        g.drawLine ((float) x, 0.0f, (float) x + 170.0f, (float) kHeight, 1.0f);

    auto drawPanel = [&g] (juce::Rectangle<float> area, const juce::String& title)
    {
        juce::ColourGradient panelGrad (juce::Colour (0xff141925), area.getTopLeft(),
                                        juce::Colour (0xff0b0e14), area.getBottomRight(), false);
        g.setGradientFill (panelGrad);
        g.fillRoundedRectangle (area, 10.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (area, 10.0f, 1.2f);
        g.setColour (Palette::gold.withAlpha (0.18f));
        g.drawRoundedRectangle (area.reduced (1.5f), 9.0f, 1.0f);

        g.setColour (Palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText (title, area.toNearestInt().reduced (12, 7), juce::Justification::topLeft);
    };

    drawPanel ({ 14.0f, 12.0f, 832.0f, 50.0f }, "PRESETS");
    drawPanel ({ 14.0f, 74.0f, 178.0f, 338.0f }, "INPUT");
    drawPanel ({ 204.0f, 74.0f, 452.0f, 338.0f }, "ENGINE");
    drawPanel ({ 668.0f, 74.0f, 178.0f, 338.0f }, "OUTPUT");
    drawPanel ({ 14.0f, 424.0f, 832.0f, 62.0f }, "CONTROL STRIP");

    // --- Header Text & Logo ---
    if (processedLogoImg.isValid())
    {
        g.drawImageWithin (processedLogoImg, 25, 14, 34, 34, juce::Justification::centred, false);
    }

    g.setColour (Palette::textBright);
    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    g.drawText ("WAPDEM", 64, 17, 92, 20, juce::Justification::left);

    g.setColour (Palette::textDim);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText ("PARALLEL CLIPPER", 64, 37, 130, 12, juce::Justification::left);

    // Version info
    g.setColour (Palette::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("v2.4.0", 790, 443, 38, 20, juce::Justification::right);
}

//==============================================================================
void WapDemSaturationEditor::resized()
{
    // ---- Header components ----
    prevPreset.setBounds (215, 24, 25, 25);
    presetName.setBounds (244, 24, 150, 25);
    nextPreset.setBounds (398, 24, 25, 25);
    savePreset.setBounds (438, 24, 52, 25);
    undoBtn   .setBounds (500, 24, 52, 25);
    redoBtn   .setBounds (560, 24, 52, 25);
    
    bypassBtn .setBounds (684, 24, 72, 25);
    loadBtn   .setBounds (766, 24, 58, 25);

    // ---- Left Panel controls ----
    inMeter   .setBounds (29, 118, 24, 250);
    inMeterLbl.setBounds (27, 94, 30, 18);

    driveKnob.setBounds (78, 105, 92, 92);
    driveLbl .setBounds (78, 199, 92, 15);

    inputKnob.setBounds (76, 224, 72, 72);
    inputLbl .setBounds (76, 298, 72, 15);

    punchKnob.setBounds (76, 323, 72, 72);
    punchLbl .setBounds (76, 394, 72, 15);

    // ---- Center Panel controls ----
    harmonicsVisualizer.setBounds (224, 105, 280, 178);

    characterLbl.setBounds (522, 112, 110, 15);
    characterBox.setBounds (522, 130, 110, 27);

    modeLbl.setBounds (522, 177, 110, 15);
    modeBox.setBounds (522, 195, 110, 27);

    oversampleLbl.setBounds (522, 242, 110, 15);
    oversampleBox.setBounds (522, 260, 110, 27);

    // ---- Right Panel controls ----
    outMeter   .setBounds (807, 118, 24, 250);
    outMeterLbl.setBounds (804, 94, 30, 18);

    mixKnob   .setBounds (696, 126, 92, 92);
    mixLbl    .setBounds (696, 220, 92, 15);

    outputKnob.setBounds (696, 272, 92, 92);
    outputLbl .setBounds (696, 366, 92, 15);

    // ---- Bottom Bar controls ----
    autoGainBtn.setBounds (36, 448, 104, 25);
    deltaBtn   .setBounds (148, 448, 76, 25);

    bandSelectLbl.setBounds (242, 432, 158, 13);
    bandLowBtn   .setBounds (244, 449, 50, 23);
    bandHighBtn  .setBounds (295, 449, 50, 23);
    bandBothBtn  .setBounds (346, 449, 52, 23);

    crossoverFreqKnob.setBounds (430, 438, 48, 48);
    crossoverFreqLbl .setBounds (410, 478, 88, 12);

    harmonicBiasKnob.setBounds (525, 438, 48, 48);
    harmonicBiasLbl .setBounds (505, 478, 88, 12);

    widthKnob.setBounds (635, 438, 48, 48);
    widthLbl .setBounds (617, 478, 84, 12);

    stereoLinkKnob.setBounds (718, 438, 48, 48);
    stereoLinkLbl .setBounds (700, 478, 84, 12);
}
