#pragma once

#include <atomic>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
//  Lock-free peak meter source. The audio thread pushes peak values; the editor
//  reads them on its timer. Ballistics (decay) are applied on read so the meter
//  falls smoothly regardless of buffer size.
//==============================================================================
class LevelMeter
{
public:
    // Called on the audio thread, once per block per channel.
    void pushBlock (const float* data, int numSamples) noexcept
    {
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));

        // Keep the loudest peak seen since the last UI read.
        float prev = pending.load (std::memory_order_relaxed);
        while (peak > prev
               && ! pending.compare_exchange_weak (prev, peak, std::memory_order_relaxed))
        { /* retry */ }
    }

    // Called on the message thread by the editor's timer.
    // Returns a smoothed 0..1 level (already converted from dB range).
    float readLevel() noexcept
    {
        const float peak = pending.exchange (0.0f, std::memory_order_relaxed);

        if (peak >= displayed)
            displayed = peak;                       // instant attack
        else
            displayed += (peak - displayed) * 0.04f; // smooth, less shaky release ballistics

        return juce::jlimit (0.0f, 1.0f, displayed);
    }

    void reset() noexcept { pending.store (0.0f); displayed = 0.0f; }

private:
    std::atomic<float> pending { 0.0f };
    float displayed { 0.0f };
};
