#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Parameters.h"

//==============================================================================
// Stable clipper-style saturation.
//
// The original Cabbage prototype was essentially tanh(input * gain). Keep this
// implementation in that family: neutral at zero drive, bounded output, subtle
// mode differences, and no aggressive harmonic re-synthesis.
//==============================================================================
class Saturation
{
public:
    void setMode (SatMode m) noexcept { mode = m; }
    void setCharacter (float character01) noexcept { characterVal = juce::jlimit (0.0f, 1.0f, character01); }
    void setClipMode (int m) noexcept { clipMode = m; }
    void setHarmonicBias (float bias01) noexcept { harmonicBias = juce::jlimit (0.0f, 1.0f, bias01); }

    void setDrive (float drive01) noexcept
    {
        driveAmount = juce::jlimit (0.0f, 1.0f, drive01);
        preGain = 1.0f + driveAmount * driveAmount * 9.0f;
    }

    void prepare (double sampleRate) noexcept
    {
        const float fc = 8.0f;
        const float wc = 2.0f * juce::MathConstants<float>::pi * fc / (float) sampleRate;
        dcAlpha = 1.0f / (1.0f + wc);
        dcX1 = 0.0f;
        dcY1 = 0.0f;
    }

    inline float processSample (float x) noexcept
    {
        if (driveAmount <= 1.0e-5f)
            return x;

        const float shaped = shapeWithBias (x * preGain);
        const float cleaned = removeDc (shaped);

        return x + driveAmount * (cleaned - x);
    }

private:
    static inline float softClip (float x) noexcept
    {
        return std::tanh (x);
    }

    static inline float hardClip (float x) noexcept
    {
        const float clipped = juce::jlimit (-1.0f, 1.0f, x);
        return 0.92f * clipped + 0.08f * std::tanh (x);
    }

    static inline float tapeClip (float x) noexcept
    {
        const float t = std::tanh (x * 0.82f);
        return t - 0.035f * t * t * t;
    }

    static inline float transformerClip (float x) noexcept
    {
        return x / (1.0f + 0.55f * std::abs (x));
    }

    inline float shapeCore (float x) const noexcept
    {
        const float tone = 0.85f + characterVal * 0.35f;

        if (clipMode == 1)
            return hardClip (x * tone);

        switch (mode)
        {
            case SatMode::Tube:
                return softClip (x * tone);
            case SatMode::ClassA:
                return softClip (x * (tone * 1.08f)) * 0.98f;
            case SatMode::Tape:
                return tapeClip (x * (0.95f + characterVal * 0.20f));
            case SatMode::Transformer:
                return transformerClip (x * (1.05f + characterVal * 0.25f));
        }

        return softClip (x);
    }

    inline float shapeWithBias (float x) const noexcept
    {
        const float bias = (harmonicBias - 0.5f) * 0.10f * driveAmount;
        return shapeCore (x + bias) - shapeCore (bias);
    }

    inline float removeDc (float x) noexcept
    {
        const float y = dcAlpha * (dcY1 + x - dcX1);
        dcX1 = x;
        dcY1 = y;
        return y;
    }

    SatMode mode { SatMode::Tube };
    float preGain { 1.0f };
    float driveAmount { 0.0f };
    float characterVal { 0.5f };
    float harmonicBias { 0.5f };
    int clipMode { 0 };

    float dcAlpha { 1.0f };
    float dcX1 { 0.0f };
    float dcY1 { 0.0f };
};
