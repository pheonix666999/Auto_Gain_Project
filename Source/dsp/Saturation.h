#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Parameters.h"

//==============================================================================
//  Musical analog-modeled saturation with:
//    - 1st-order ADAA (Antiderivative Anti-Aliasing) on all waveshapers
//    - DC blocker to remove asymmetric-curve offset
//    - Harmonic bias control (even ↔ odd harmonic blend)
//    - Subtle, gradual onset — musical at all drive levels
//==============================================================================
class Saturation
{
public:
    void setMode (SatMode m) noexcept { mode = m; }
    void setCharacter (float character01) noexcept { characterVal = character01; }
    void setClipMode (int m) noexcept { clipMode = m; }
    void setHarmonicBias (float bias01) noexcept { harmonicBias = bias01; }

    // drive01 : 0..1  — scaled for musical, gradual saturation onset
    void setDrive (float drive01) noexcept
    {
        driveAmount = juce::jlimit (0.0f, 1.0f, drive01);
        // Much gentler pre-gain curve than before: at drive=1.0 we get ~6x gain
        // (old was 1+d²*24 = up to 25x — way too harsh)
        preGain = 1.0f + driveAmount * driveAmount * 5.0f;
    }

    void prepare (double sampleRate) noexcept
    {
        // DC blocker: 1st-order highpass at ~6 Hz
        const float fc = 6.0f;
        const float wc = 2.0f * juce::MathConstants<float>::pi * fc / (float) sampleRate;
        dcAlpha = 1.0f / (1.0f + wc);
        dcX1 = 0.0f;
        dcY1 = 0.0f;

        // ADAA state
        x1 = 0.0f;
    }

    inline float processSample (float x) noexcept
    {
        if (driveAmount <= 1.0e-5f)
            return x;

        const float in = x * preGain;

        // --- 1st-order ADAA: F(x[n]) = (AD(x[n]) - AD(x[n-1])) / (x[n] - x[n-1]) ---
        float sat;
        const float dx = in - x1;
        if (std::abs (dx) > 1.0e-5f)
        {
            sat = (antiderivative (in) - antiderivative (x1)) / dx;
        }
        else
        {
            // Fall back to direct evaluation when samples are too close
            sat = waveshape (in);
        }
        x1 = in;

        // --- Soft clip safety (prevents runaway from extreme input) ---
        if (clipMode == 1) // Hard Clip
            sat = juce::jlimit (-1.0f, 1.0f, sat);
        else // Soft clip — gentle limiter at ±1.2
            sat = std::tanh (sat * 0.833f) * 1.2f;

        // --- DC blocker ---
        const float dcOut = dcAlpha * (dcY1 + sat - dcX1);
        dcX1 = sat;
        dcY1 = dcOut;

        return x + driveAmount * (dcOut - x);
    }

private:
    //==========================================================================
    //  Core waveshaping function — selects mode and applies harmonic bias
    //==========================================================================
    inline float waveshape (float x) const noexcept
    {
        // Blend between even-harmonic (asymmetric) and odd-harmonic (symmetric) shapes
        // harmonicBias: 0 = full even, 0.5 = balanced, 1.0 = full odd
        const float evenWeight = 1.0f - harmonicBias;
        const float oddWeight  = harmonicBias;

        float even = 0.0f;
        float odd  = 0.0f;

        switch (mode)
        {
            case SatMode::Tube:
                even = tubeEven (x, characterVal);
                odd  = tubeOdd (x, characterVal);
                break;
            case SatMode::ClassA:
                even = classAEven (x, characterVal);
                odd  = classAOdd (x, characterVal);
                break;
            case SatMode::Tape:
                even = tapeEven (x, characterVal);
                odd  = tapeOdd (x, characterVal);
                break;
            case SatMode::Transformer:
                even = transformerEven (x, characterVal);
                odd  = transformerOdd (x, characterVal);
                break;
        }

        return even * evenWeight + odd * oddWeight;
    }

    //==========================================================================
    //  Antiderivative of the waveshaping function (for ADAA)
    //  Uses analytical antiderivatives where possible, numerical otherwise
    //==========================================================================
    inline float antiderivative (float x) const noexcept
    {
        // For the blended waveshaper, compute AD of each component
        const float evenWeight = 1.0f - harmonicBias;
        const float oddWeight  = harmonicBias;

        float adEven = 0.0f;
        float adOdd  = 0.0f;

        switch (mode)
        {
            case SatMode::Tube:
                adEven = adTubeEven (x, characterVal);
                adOdd  = adTubeOdd (x, characterVal);
                break;
            case SatMode::ClassA:
                adEven = adClassAEven (x, characterVal);
                adOdd  = adClassAOdd (x, characterVal);
                break;
            case SatMode::Tape:
                adEven = adTapeEven (x, characterVal);
                adOdd  = adTapeOdd (x, characterVal);
                break;
            case SatMode::Transformer:
                adEven = adTransformerEven (x, characterVal);
                adOdd  = adTransformerOdd (x, characterVal);
                break;
        }

        return adEven * evenWeight + adOdd * oddWeight;
    }

    //==========================================================================
    //  TUBE — Triode vacuum tube emulation
    //==========================================================================

    // Even harmonics: asymmetric curve with DC bias
    static inline float tubeEven (float x, float ch) noexcept
    {
        const float bias = 0.05f + ch * 0.15f;
        const float in = x + bias;
        // Asymmetric: positive side clips more gently
        float out;
        if (in > 0.0f)
            out = 1.0f - std::exp (-in * 1.2f);
        else
            out = -1.0f + std::exp (in * 0.8f);
        return out * 0.85f;
    }

    static inline float adTubeEven (float x, float ch) noexcept
    {
        const float bias = 0.05f + ch * 0.15f;
        const float in = x + bias;
        // AD of piecewise: ∫(1 - e^(-1.2*in)) = in + e^(-1.2*in)/1.2  for in>0
        //                  ∫(-1 + e^(0.8*in))  = -in + e^(0.8*in)/0.8  for in<0
        float ad;
        if (in > 0.0f)
            ad = in + std::exp (-in * 1.2f) / 1.2f;
        else
            ad = -in + std::exp (in * 0.8f) / 0.8f;
        return ad * 0.85f;
    }

    // Odd harmonics: symmetric tanh
    static inline float tubeOdd (float x, float ch) noexcept
    {
        const float k = 1.0f + ch * 0.5f;
        return std::tanh (x * k);
    }

    static inline float adTubeOdd (float x, float ch) noexcept
    {
        // AD of tanh(kx) = ln(cosh(kx)) / k
        const float k = 1.0f + ch * 0.5f;
        // log(cosh(x)) = |x| + log(1 + exp(-2|x|)) - log(2) for numerical stability
        const float kx = x * k;
        const float akx = std::abs (kx);
        return (akx + std::log1p (std::exp (-2.0f * akx)) - 0.6931472f) / k;
    }

    //==========================================================================
    //  CLASS A — Germanium / FET single-ended preamp
    //==========================================================================

    // Even: soft asymmetric knee with 2nd harmonic emphasis
    static inline float classAEven (float x, float ch) noexcept
    {
        const float asym = 0.03f + ch * 0.08f;
        const float in = x + asym;
        // Soft polynomial with asymmetry
        const float x2 = in * in;
        return in / (1.0f + 0.25f * x2) + 0.15f * ch * x2 / (1.0f + x2);
    }

    static inline float adClassAEven (float x, float ch) noexcept
    {
        // Approximate AD numerically using Simpson-like midpoint
        const float h = 0.001f;
        float sum = 0.0f;
        const int n = (int)(std::abs(x) / h);
        const float sign = x >= 0.0f ? 1.0f : -1.0f;
        const float absX = std::abs(x);
        // Use trapezoidal approximation for numerical AD
        // AD(x) ≈ ∫₀ˣ f(t)dt
        float t = 0.0f;
        const float step = absX / juce::jmax(n, 1);
        for (int i = 0; i <= juce::jmin(n, 200); ++i)
        {
            float w = (i == 0 || i == juce::jmin(n, 200)) ? 0.5f : 1.0f;
            sum += w * classAEven(sign * t, ch);
            t += step;
        }
        return sign * sum * step;
    }

    // Odd: symmetric soft clip
    static inline float classAOdd (float x, float ch) noexcept
    {
        const float k = 1.2f + ch * 0.3f;
        return std::tanh (x * k) * 0.9f;
    }

    static inline float adClassAOdd (float x, float ch) noexcept
    {
        const float k = 1.2f + ch * 0.3f;
        const float kx = x * k;
        const float akx = std::abs (kx);
        return 0.9f * (akx + std::log1p (std::exp (-2.0f * akx)) - 0.6931472f) / k;
    }

    //==========================================================================
    //  TAPE — Magnetic tape saturation
    //==========================================================================

    // Even: subtle 2nd harmonic from tape head bias
    static inline float tapeEven (float x, float ch) noexcept
    {
        const float bias = 0.02f + ch * 0.06f;
        const float in = x + bias;
        return std::tanh (in) * 0.92f;
    }

    static inline float adTapeEven (float x, float ch) noexcept
    {
        const float bias = 0.02f + ch * 0.06f;
        const float in = x + bias;
        const float ain = std::abs (in);
        return 0.92f * (ain + std::log1p (std::exp (-2.0f * ain)) - 0.6931472f);
    }

    // Odd: strong 3rd harmonic compression — the tape sound
    static inline float tapeOdd (float x, float ch) noexcept
    {
        const float drive = 1.0f + ch * 0.4f;
        // tanh with slight cubic warmth
        const float t = std::tanh (x * drive);
        return (t - 0.05f * t * t * t) * 0.95f;
    }

    static inline float adTapeOdd (float x, float ch) noexcept
    {
        const float drive = 1.0f + ch * 0.4f;
        const float kx = x * drive;
        const float akx = std::abs (kx);
        // AD of tanh(kx) part
        float adTanh = (akx + std::log1p (std::exp (-2.0f * akx)) - 0.6931472f) / drive;
        // The cubic correction is small; approximate its AD
        // AD of -0.05*tanh³(kx) ≈ -0.05 * [tanh(kx) - tanh(kx)/3] / k (rough)
        // For simplicity, we use the tanh AD as dominant term
        return adTanh * 0.95f;
    }

    //==========================================================================
    //  TRANSFORMER — Magnetic core / iron saturation
    //==========================================================================

    // Even: slight asymmetry from core magnetization
    static inline float transformerEven (float x, float ch) noexcept
    {
        const float bias = 0.02f + ch * 0.05f;
        const float in = x + bias;
        const float absIn = std::abs (in);
        return in / (1.0f + absIn * 0.6f + 0.12f * in * in);
    }

    static inline float adTransformerEven (float x, float ch) noexcept
    {
        // Numerical AD for the rational function
        const float bias = 0.02f + ch * 0.05f;
        const float in = x + bias;
        const float absIn = std::abs (in);
        // Approximate: ∫ x/(1 + |x|*0.6 + 0.12*x²) dx
        // Use log approximation of the denominator
        const float denom = 1.0f + absIn * 0.6f + 0.12f * in * in;
        return 0.5f * std::log (denom) / 0.12f * 0.5f; // rough analytical fit
    }

    // Odd: symmetric iron saturation — the "weight"
    static inline float transformerOdd (float x, float ch) noexcept
    {
        const float stiffness = 0.1f + ch * 0.15f;
        const float absX = std::abs (x);
        return x / (1.0f + absX + stiffness * x * x);
    }

    static inline float adTransformerOdd (float x, float ch) noexcept
    {
        // AD of x/(1 + |x| + s*x²): use log approximation
        const float stiffness = 0.1f + ch * 0.15f;
        const float absX = std::abs (x);
        const float denom = 1.0f + absX + stiffness * x * x;
        return 0.5f * std::log (denom) / (1.0f + stiffness);
    }

    //==========================================================================
    SatMode mode { SatMode::Tube };
    float preGain { 1.0f };
    float driveAmount { 0.0f };
    float characterVal { 0.5f };
    float harmonicBias { 0.5f };   // 0 = even, 0.5 = balanced, 1 = odd
    int clipMode { 0 };

    // ADAA state
    float x1 { 0.0f };

    // DC blocker state
    float dcAlpha { 1.0f };
    float dcX1 { 0.0f };
    float dcY1 { 0.0f };
};
