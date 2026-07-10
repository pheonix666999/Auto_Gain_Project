#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WapDemSaturationProcessor::WapDemSaturationProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "PARAMETERS", createParameterLayout())
{
}

//==============================================================================
void WapDemSaturationProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 2;

    // 1 stage = 2x, 2 stages = 4x, 3 stages = 8x. FIR for linear phase / lowest aliasing.
    oversampler2x = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampler4x = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampler8x = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);

    oversampler2x->initProcessing ((size_t) samplesPerBlock);
    oversampler4x->initProcessing ((size_t) samplesPerBlock);
    oversampler8x->initProcessing ((size_t) samplesPerBlock);

    tone.prepare (spec);
    tone.reset();

    auto smoothPrep = [sampleRate] (juce::SmoothedValue<float>& s, float init)
    {
        s.reset (sampleRate, 0.02);   // 20 ms ramp -> click-free
        s.setCurrentAndTargetValue (init);
    };
    smoothPrep (inputSm,  1.0f);
    smoothPrep (driveSm,  0.0f);
    smoothPrep (toneSm,   50.0f);
    smoothPrep (bassSm,   0.0f);
    smoothPrep (characterSm, 0.5f);
    smoothPrep (outputSm, 1.0f);
    smoothPrep (hissSm,   0.0f);
    smoothPrep (punchSm,  0.5f);
    smoothPrep (crossoverFreqSm, 500.0f);
    smoothPrep (stereoLinkSm,    100.0f);
    smoothPrep (widthSm,         100.0f);
    smoothPrep (harmonicBiasSm,  0.5f);
    smoothPrep (autoGainCompSm,  1.0f);

    for (auto& s : saturation)
        s.prepare (sampleRate);

    for (int ch = 0; ch < 2; ++ch)
        crossover[ch].prepare (spec);

    for (auto& m : inMeter)  m.reset();
    for (auto& m : outMeter) m.reset();

    setLatencySamples (0);
}

//==============================================================================
bool WapDemSaturationProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out) return false;
    return in == juce::AudioChannelSet::mono()
        || in == juce::AudioChannelSet::stereo();
}

//==============================================================================
OverSampleMode WapDemSaturationProcessor::currentOversampleMode() const noexcept
{
    return (OverSampleMode) apvts.getRawParameterValue (ParamID::oversample)->load();
}

void WapDemSaturationProcessor::updateParameters()
{
    inputSm .setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue (ParamID::input)->load()));
    driveSm .setTargetValue (apvts.getRawParameterValue (ParamID::drive) ->load() * 0.01f);
    toneSm  .setTargetValue (apvts.getRawParameterValue (ParamID::tone)  ->load());
    bassSm  .setTargetValue (apvts.getRawParameterValue (ParamID::bass)  ->load());
    characterSm.setTargetValue (apvts.getRawParameterValue (ParamID::character)->load() * 0.01f);
    mixSm   .setTargetValue (apvts.getRawParameterValue (ParamID::mix)   ->load() * 0.01f);
    
    // Output knob is -12 to +12 dB
    const float outKnob = apvts.getRawParameterValue (ParamID::output)->load();
    outputSm.setTargetValue (juce::Decibels::decibelsToGain (outKnob));
    hissSm  .setTargetValue (apvts.getRawParameterValue (ParamID::hiss) ->load() * 0.01f);
    punchSm .setTargetValue (apvts.getRawParameterValue (ParamID::punch)->load() * 0.01f);
    crossoverFreqSm.setTargetValue (apvts.getRawParameterValue (ParamID::crossoverFreq)->load());
    stereoLinkSm   .setTargetValue (apvts.getRawParameterValue (ParamID::stereoLink)->load());
    widthSm        .setTargetValue (apvts.getRawParameterValue (ParamID::width)->load());
    harmonicBiasSm .setTargetValue (apvts.getRawParameterValue (ParamID::harmonicBias)->load() * 0.01f);

    const float freq = crossoverFreqSm.getTargetValue();
    for (int ch = 0; ch < 2; ++ch)
        crossover[ch].setCutoffFrequency (freq);

    const auto mode = (SatMode) (int) apvts.getRawParameterValue (ParamID::mode)->load();
    const int clip = (int) apvts.getRawParameterValue (ParamID::clipMode)->load();
    const float charVal = characterSm.getTargetValue();
    const float biasVal = harmonicBiasSm.getTargetValue();
    for (auto& s : saturation)
    {
        s.setMode (mode);
        s.setClipMode (clip);
        s.setCharacter (charVal);
        s.setHarmonicBias (biasVal);
    }
}

//==============================================================================
void WapDemSaturationProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    updateParameters();

    const bool bypass = apvts.getRawParameterValue (ParamID::bypass)->load() > 0.5f;
    const bool active = apvts.getRawParameterValue (ParamID::active)->load() > 0.5f;

    if (bypass || ! active)
    {
        for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
        {
            inMeter[ch].pushBlock (buffer.getReadPointer (ch), numSamples);
            outMeter[ch].pushBlock (buffer.getReadPointer (ch), numSamples);
        }
        return;
    }

    // ----- Apply Input Gain -------------------------------------------------
    for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        auto inputCopy = inputSm;
        for (int i = 0; i < numSamples; ++i)
            d[i] *= inputCopy.getNextValue();
    }
    inputSm.skip (numSamples);

    // ----- input metering (pre-process) -------------------------------------
    for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
        inMeter[ch].pushBlock (buffer.getReadPointer (ch), numSamples);

    // Keep a clean copy for the parallel (dry) path.
    juce::AudioBuffer<float> dry;
    dry.makeCopyOf (buffer);

    // ----- choose oversampler & report latency ------------------------------
    const auto osMode = currentOversampleMode();
    juce::dsp::Oversampling<float>* os = nullptr;
    if      (osMode == OverSampleMode::x2) os = oversampler2x.get();
    else if (osMode == OverSampleMode::x4) os = oversampler4x.get();
    else if (osMode == OverSampleMode::x8) os = oversampler8x.get();

    if (osMode != activeOversample)
    {
        activeOversample = osMode;
        const int lat = os ? (int) os->getLatencyInSamples() : 0;
        setLatencySamples (lat);
    }

    // ----- update tone coefficients once per block --------------------------
    tone.update (toneSm.getTargetValue(), bassSm.getTargetValue());

    // ----- drive: advance the smoother once per block (rate-independent) -----
    const float driveVal = driveSm.skip (numSamples);
    for (auto& sat : saturation) sat.setDrive (driveVal);

    // ----- saturation (optionally oversampled) ------------------------------
    juce::dsp::AudioBlock<float> block (buffer);

    auto runShaper = [this] (juce::dsp::AudioBlock<float>& b)
    {
        const int n  = (int) b.getNumSamples();
        const int ch = (int) b.getNumChannels();
        const float punchVal = punchSm.getTargetValue();
        const double sr = this->spec.sampleRate > 0 ? this->spec.sampleRate : 48000.0;
        
        // Attack (3ms) & Release (80ms) envelope times
        const float att = std::exp (-1.0f / (float)(sr * 0.003f));
        const float rel = std::exp (-1.0f / (float)(sr * 0.080f));

        const int bandChoice = (int) apvts.getRawParameterValue (ParamID::bandSelect)->load();

        if (ch == 2)
        {
            auto* dL = b.getChannelPointer (0);
            auto* dR = b.getChannelPointer (1);
            auto& satL = saturation[0];
            auto& satR = saturation[1];
            float envL = punchEnv[0];
            float envR = punchEnv[1];

            for (int i = 0; i < n; ++i)
            {
                float inL = dL[i];
                float inR = dR[i];

                float lowL = 0.0f, highL = 0.0f;
                crossover[0].processSample (0, inL, lowL, highL);
                
                float lowR = 0.0f, highR = 0.0f;
                crossover[1].processSample (1, inR, lowR, highR);

                float targetL = (bandChoice == 0) ? lowL : ((bandChoice == 1) ? highL : inL);
                float targetR = (bandChoice == 0) ? lowR : ((bandChoice == 1) ? highR : inR);

                float absInL = std::abs (targetL);
                float absInR = std::abs (targetR);

                if (absInL > envL) envL = absInL + att * (envL - absInL);
                else               envL = absInL + rel * (envL - absInL);

                if (absInR > envR) envR = absInR + att * (envR - absInR);
                else               envR = absInR + rel * (envR - absInR);

                float linkAmount = stereoLinkSm.getTargetValue() * 0.01f;
                float maxEnv = juce::jmax (envL, envR);
                float linkedEnvL = envL + linkAmount * (maxEnv - envL);
                float linkedEnvR = envR + linkAmount * (maxEnv - envR);

                float punchModL = 1.0f;
                float punchModR = 1.0f;
                if (punchVal > 0.5f)
                {
                    float transL = juce::jmax (0.0f, absInL - linkedEnvL);
                    float transR = juce::jmax (0.0f, absInR - linkedEnvR);
                    punchModL = 1.0f + transL * (punchVal - 0.5f) * 4.0f;
                    punchModR = 1.0f + transR * (punchVal - 0.5f) * 4.0f;
                }
                else
                {
                    punchModL = 1.0f - linkedEnvL * (0.5f - punchVal) * 0.8f;
                    punchModR = 1.0f - linkedEnvR * (0.5f - punchVal) * 0.8f;
                    if (punchModL < 0.2f) punchModL = 0.2f;
                    if (punchModR < 0.2f) punchModR = 0.2f;
                }

                float satLVal = satL.processSample (targetL * punchModL);
                float satRVal = satR.processSample (targetR * punchModR);

                if (bandChoice == 0)
                {
                    dL[i] = satLVal + highL;
                    dR[i] = satRVal + highR;
                }
                else if (bandChoice == 1)
                {
                    dL[i] = lowL + satLVal;
                    dR[i] = lowR + satRVal;
                }
                else
                {
                    dL[i] = satLVal;
                    dR[i] = satRVal;
                }
            }
            punchEnv[0] = envL;
            punchEnv[1] = envR;
        }
        else
        {
            for (int c = 0; c < ch; ++c)
            {
                auto* d = b.getChannelPointer ((size_t) c);
                auto& sat = saturation[juce::jmin (c, 1)];
                auto& xover = crossover[juce::jmin (c, 1)];
                float env = punchEnv[juce::jmin (c, 1)];

                for (int i = 0; i < n; ++i)
                {
                    float inVal = d[i];
                    float low = 0.0f, high = 0.0f;
                    xover.processSample (juce::jmin (c, 1), inVal, low, high);

                    float target = (bandChoice == 0) ? low : ((bandChoice == 1) ? high : inVal);
                    float absIn = std::abs (target);
                    
                    if (absIn > env)
                        env = absIn + att * (env - absIn);
                    else
                        env = absIn + rel * (env - absIn);
                    
                    float punchMod = 1.0f;
                    if (punchVal > 0.5f)
                    {
                        float trans = juce::jmax (0.0f, absIn - env);
                        punchMod = 1.0f + trans * (punchVal - 0.5f) * 4.0f;
                    }
                    else
                    {
                        punchMod = 1.0f - env * (0.5f - punchVal) * 0.8f;
                        if (punchMod < 0.2f) punchMod = 0.2f;
                    }

                    float satVal = sat.processSample (target * punchMod);
                    if (bandChoice == 0)
                        d[i] = satVal + high;
                    else if (bandChoice == 1)
                        d[i] = low + satVal;
                    else
                        d[i] = satVal;
                }
                punchEnv[juce::jmin (c, 1)] = env;
            }
        }
    };

    if (os != nullptr)
    {
        auto osBlock = os->processSamplesUp (block);
        runShaper (osBlock);
        os->processSamplesDown (block);
    }
    else
    {
        runShaper (block);
    }

    // ----- tone shaping (at base rate) --------------------------------------
    juce::dsp::ProcessContextReplacing<float> toneCtx (block);
    tone.process (toneCtx);

    // ----- tape hiss (only in Tape mode) --------------------------
    const auto mode = (SatMode) (int) apvts.getRawParameterValue (ParamID::mode)->load();
    if (mode == SatMode::Tape)
    {
        for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const float h = hissSm.getNextValue();
                if (h > 0.0f)
                    d[i] += (hissRng.nextFloat() * 2.0f - 1.0f) * 0.0025f * h;
            }
        }
    }

    // ----- parallel mix + output gain + output metering ---------
    const bool autoGain = apvts.getRawParameterValue (ParamID::autoGain)->load() > 0.5f;
    const bool delta = apvts.getRawParameterValue (ParamID::delta)->load() > 0.5f;
    
    float autoGainTarget = 1.0f;
    if (autoGain)
    {
        double dryEnergy = 0.0;
        double wetEnergy = 0.0;
        const int measuredChannels = juce::jmin (numCh, 2);

        for (int ch = 0; ch < measuredChannels; ++ch)
        {
            dryEnergy += dry.getRMSLevel (ch, 0, numSamples);
            wetEnergy += buffer.getRMSLevel (ch, 0, numSamples);
        }

        const float dryRms = measuredChannels > 0 ? (float) (dryEnergy / measuredChannels) : 0.0f;
        const float wetRms = measuredChannels > 0 ? (float) (wetEnergy / measuredChannels) : 0.0f;

        if (wetRms > 1.0e-5f)
            autoGainTarget = juce::jlimit (0.25f, 4.0f, dryRms / wetRms);
    }

    autoGainCompSm.setTargetValue (autoGainTarget);

    if (numCh == 2)
    {
        auto* wetL = buffer.getWritePointer (0);
        auto* wetR = buffer.getWritePointer (1);
        auto* dL   = dry.getReadPointer (0);
        auto* dR   = dry.getReadPointer (1);

        auto mixCopy = mixSm;
        auto outCopy = outputSm;
        auto widthCopy = widthSm;
        auto autoGainCopy = autoGainCompSm;

        for (int i = 0; i < numSamples; ++i)
        {
            const float m = mixCopy.getNextValue();
            const float g = outCopy.getNextValue();
            const float w = widthCopy.getNextValue() * 0.01f;
            const float autoGainComp = autoGainCopy.getNextValue();

            float wetCompL = wetL[i] * autoGainComp;
            float wetCompR = wetR[i] * autoGainComp;

            // Apply Mid-Side width scaling to wet signal
            float mid = 0.5f * (wetCompL + wetCompR);
            float side = 0.5f * (wetCompL - wetCompR);
            side *= w;
            wetCompL = mid + side;
            wetCompR = mid - side;

            if (delta)
            {
                wetL[i] = (wetCompL - dL[i]) * m * g;
                wetR[i] = (wetCompR - dR[i]) * m * g;
            }
            else
            {
                wetL[i] = (dL[i] * (1.0f - m) + wetCompL * m) * g;
                wetR[i] = (dR[i] * (1.0f - m) + wetCompR * m) * g;
            }
        }
        stereoLinkSm.skip (numSamples);
        widthSm     .skip (numSamples);

        inMeter[0].pushBlock (dry.getReadPointer (0), numSamples);
        inMeter[1].pushBlock (dry.getReadPointer (1), numSamples);
        outMeter[0].pushBlock (buffer.getReadPointer (0), numSamples);
        outMeter[1].pushBlock (buffer.getReadPointer (1), numSamples);
    }
    else
    {
        for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            auto* d   = dry.getReadPointer (ch);

            auto mixCopy = mixSm;
            auto outCopy = outputSm;
            auto autoGainCopy = autoGainCompSm;

            for (int i = 0; i < numSamples; ++i)
            {
                const float m = mixCopy.getNextValue();
                const float g = outCopy.getNextValue();
                const float autoGainComp = autoGainCopy.getNextValue();
                float wetComp = wet[i] * autoGainComp;

                if (delta)
                {
                    wet[i] = (wetComp - d[i]) * m * g;
                }
                else
                {
                    wet[i] = (d[i] * (1.0f - m) + wetComp * m) * g;
                }
            }

            inMeter[ch].pushBlock (dry.getReadPointer (ch), numSamples);
            outMeter[ch].pushBlock (buffer.getReadPointer (ch), numSamples);
        }
    }

    bassSm.skip (numSamples);
    characterSm.skip (numSamples);
    mixSm.skip (numSamples);
    outputSm.skip (numSamples);
    crossoverFreqSm.skip (numSamples);
    harmonicBiasSm.skip (numSamples);
    autoGainCompSm.skip (numSamples);
}

//==============================================================================
//  Programs / factory presets
//==============================================================================
struct FactoryPreset { const char* name; float drive, tone, bass, character, output, mix; int mode, os; };

static const FactoryPreset kFactoryPresets[] =
{
    { "Default Preset",      0, 50,  0.0f, 50.0f,  0.0f, 100, 0, 1 }, // Tube
    { "Gentle Warmth",      18, 55,  1.0f, 40.0f, -0.5f,  60, 0, 1 }, // Tube
    { "Crushed Drums",      75, 45,  2.0f, 75.0f, -2.0f, 100, 3, 2 }, // Transformer
    { "Vintage Tape",       40, 40,  1.5f, 50.0f, -0.5f,  80, 2, 1 }, // Tape
    { "Dub Sound System",   60, 35,  4.0f, 60.0f, -1.0f,  90, 2, 2 }, // Tape
    { "Bright Edge",        50, 70, -2.0f, 30.0f,  0.0f, 100, 1, 1 }, // Class A
};

int WapDemSaturationProcessor::getNumPrograms() { return (int) std::size (kFactoryPresets); }
int WapDemSaturationProcessor::getCurrentProgram() { return currentProgram; }
const juce::String WapDemSaturationProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) std::size (kFactoryPresets)))
        return kFactoryPresets[index].name;
    return {};
}

void WapDemSaturationProcessor::setCurrentProgram (int index) { loadFactoryPreset (index); }

void WapDemSaturationProcessor::loadFactoryPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) std::size (kFactoryPresets)))
        return;

    currentProgram = index;
    const auto& p = kFactoryPresets[index];

    auto set = [this] (const juce::String& id, float v)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (v));
    };

    set (ParamID::drive,  p.drive);
    set (ParamID::tone,   p.tone);
    set (ParamID::bass,   p.bass);
    set (ParamID::character, p.character);
    set (ParamID::output, p.output);
    set (ParamID::mix,    p.mix);
    set (ParamID::mode,   (float) p.mode);
    set (ParamID::oversample, (float) p.os);
    set (ParamID::crossoverFreq, 500.0f);
    set (ParamID::bandSelect,   2.0f); // Both
    set (ParamID::clipMode,     0.0f); // Soft Clip
    set (ParamID::autoGain,     1.0f);
}

//==============================================================================
juce::AudioProcessorEditor* WapDemSaturationProcessor::createEditor()
{
    return new WapDemSaturationEditor (*this);
}

//==============================================================================
void WapDemSaturationProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        xml->setAttribute ("program", currentProgram);
        copyXmlToBinary (*xml, destData);
    }
}

void WapDemSaturationProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            currentProgram = xml->getIntAttribute ("program", 0);
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WapDemSaturationProcessor();
}
