# Client DSP Change Log

This document records the corrective DSP changes made for the client feedback about load unity, Mix behavior, harsh saturation/harmonics, low-end preservation, and Auto Gain.

## Client Requirements Addressed

- The plugin must not change the sound immediately after loading.
- The default state must be unity gain.
- The Mix knob must not behave like an output volume control.
- At Mix 0, the plugin must pass the dry signal instead of lowering audio.
- The saturation should stay closer to the original clipper-style behavior.
- Harmonics and saturation modes should not sound harsh or disturbing.
- Auto Gain must reduce the need to balance Output and Mix manually.
- Low-end preservation must keep bass energy from being over-clipped.
- Tape hiss/noise must not be active by default.

## Files Changed

- `Source/PluginProcessor.cpp`
- `Source/Parameters.h`
- `Source/dsp/Saturation.h`

## Processing And Gain-Staging Changes

### Neutral Load / Unity Startup

Added a neutral-load passthrough check in `PluginProcessor::processBlock`.

When these parameters are neutral, processing returns immediately and leaves the audio untouched:

- `Input = 0 dB`
- `Drive = 0`
- `Tone = 50`
- `Bass = 0 dB`
- `Output = 0 dB`
- `Tape Hiss = 0`
- `Delta = off`

This prevents tone filters, oversampling, saturation, low-end split processing, Auto Gain, or output gain from changing the signal before the user touches a control.

### Drive-Zero DSP Bypass

The saturation, oversampling, and low-end split path now runs only when Drive is above zero.

This means:

- `Drive = 0` does not run clipper processing.
- `Drive = 0` does not run oversampling.
- `Drive = 0` does not run the low/high crossover split.
- The plugin is safer when other controls are neutral or when the user pulls Drive down completely.

### Mix Knob Behavior

The Mix stage now works as a true dry/wet blend:

```cpp
output = dry * (1.0f - mix) + wet * mix;
```

This means:

- `Mix = 0%` is dry signal.
- `Mix = 100%` is fully processed signal.
- Mix no longer behaves like a hidden volume control.
- Delta mode still uses Mix as the amount of difference signal.

### Auto Gain

Auto Gain is enabled by default and compares dry RMS against wet RMS.

The wet path is compensated before dry/wet mixing:

```cpp
wetComp = wet * autoGainComp;
```

The compensation is clamped to a safe range so it cannot apply extreme gain jumps:

- Minimum: `0.25x`
- Maximum: `4.0x`

This reduces the constant back-and-forth between Output and Mix that the client complained about.

## Saturation Algorithm Changes

### Simpler Clipper-Style Core

The saturation was corrected toward the original prototype behavior: a bounded clipper-style curve based around `tanh`.

The current design prioritizes:

- Unity at zero Drive.
- Smooth transition from dry to clipped signal.
- Bounded output.
- Less aggressive harmonic generation.
- Subtle differences between modes instead of radically different or harsh sounds.

### Zero-Drive Passthrough In Saturation

`Saturation::processSample` now returns the input sample unchanged when Drive is effectively zero.

This protects against accidental coloration from the saturation class itself.

### Harmonic Bias Softened

The harmonic bias amount is now intentionally small and scaled by Drive.

This prevents the harmonic control from dominating the sound or creating harsh asymmetry at low settings.

### Saturation Modes Stabilized

The modes now use restrained shaping curves:

- Tube: smooth `tanh` soft clipping.
- Class A: slightly stronger `tanh` curve.
- Tape: softer tape-style curve.
- Transformer: smooth bounded transformer-style curve.
- Hard Clip mode: limited hard clipping blended with a small `tanh` component.

## Low-End Preservation Changes

The low-end preservation crossover range was changed to match the requested sidechain/filter concept:

- Minimum: `20 Hz`
- Maximum: `250 Hz`
- Default: `120 Hz`

The split runs at base sample rate before oversampling so the crossover does not operate at the wrong sample rate inside the oversampled path.

When the band selector is set to process the high band or both bands, the low portion is preserved and added back after clipping. This keeps bass and sub energy from being unnecessarily destroyed by the clipper.

## Noise / Hiss Changes

Tape Hiss now defaults to `0%`.

Hiss is only added when:

- Tape mode is selected.
- The Tape Hiss parameter is above zero.

The hiss smoother is now advanced once per block consistently, preventing mono/stereo behavior differences.

## Factory Preset Safety Changes

Factory preset loading now resets these safety-critical parameters:

- `Input = 0 dB`
- `Tape Hiss = 0`
- `Punch = 50`
- `Crossover Frequency = 120 Hz`
- `Band Select = Both`
- `Clip Mode = Soft Clip`
- `Delta = off`
- `Harmonic Bias = 50`
- `Stereo Link = 100`
- `Width = 100`
- `Auto Gain = on`

This prevents a preset from accidentally leaving the plugin in a non-unity or noisy state.

## Smoother / State Fixes

Parameter smoothers are now advanced consistently for:

- Mix
- Output
- Hiss
- Crossover Frequency
- Harmonic Bias
- Stereo Link
- Width
- Auto Gain compensation
- Punch

This avoids stale smoother state when switching between mono/stereo, Tape/non-Tape, or neutral/processed states.

## Control Cleanup / Follow-Up Fixes

### Punch Now Works

The Punch knob is now connected to the audio path.

It is implemented as a centered transient contour on the wet path:

- `50%` is neutral.
- Above `50%` adds transient attack.
- Below `50%` softens transient attack.

The neutral-load passthrough now checks that Punch is at `50%`, so moving Punch exits the passthrough path and produces an audible change.

### Duplicate Mix Knob Removed

The duplicate left-panel Mix knob was removed from the editor.

There is now only one visible Mix knob, attached to the single `mix` parameter. This avoids the confusing previous state where two separate UI knobs controlled the exact same parameter.

### Width And Link Now Affect Audio

The Width knob now applies stereo width after the dry/wet mix.

The Link knob now affects the wet stereo path before mixing:

- `100%` is neutral/default.
- Lower values progressively unlink/widen the wet stereo image.

The neutral-load passthrough now checks that Width and Link are at their default values, so moving either control exits passthrough and affects the audio.

### Header Buttons Wired

Previously visible header buttons that had no action are now wired:

- Save writes the current APVTS state to an XML preset.
- Undo calls the processor undo manager.
- Redo calls the processor undo manager.

## GUI And Delivery Cleanup

### Compact Professional Layout

The editor was tightened from a wide `1000x600` layout to a compact `860x500` layout.

The controls are now grouped into clear sections:

- Input
- Engine
- Output
- Control Strip

This removes the excessive empty spacing and makes the plugin feel more finished.

### New Center Display

The oversized harmonic bar display was replaced with a compact parallel saturation core display.

The new display shows:

- Dry/wet split
- Active saturation type
- Soft/hard clip mode
- Low-end preservation band
- Drive curve
- Punch amount
- Harmonic motion

### Clearer Saturation Labels

The two saturation selectors were relabeled for clarity:

- `SAT TYPE` controls Tube, Class A, Tape, and Transformer.
- `CLIP MODE` controls Soft Clip and Hard Clip.

### Cleaner Delivery Artifacts

GitHub Actions now uploads only the platform zip instead of uploading both:

- the zip
- the extracted folder containing the same files

This prevents the client from opening an artifact and seeing what looks like two copies of the same plugin.

### Security Warning Note

`DELIVERY_NOTES.md` was added and is copied into each package.

It explains that unsigned Windows `.exe` and plugin binaries can trigger SmartScreen or antivirus warnings, and that public release builds should be code-signed.

## Verification Performed

The following source-level checks were performed:

```bash
git diff --check -- Source/PluginProcessor.cpp Source/Parameters.h Source/dsp/Saturation.h
```

Result: passed.

The source was also searched to confirm the expected correction points are present:

- `neutralLoadState`
- `driveVal > 1.0e-5f`
- `autoGainTarget`
- `Tape Hiss`
- `Crossover Frequency`
- `Stable clipper-style saturation`

## Verification Not Performed Locally

Runtime audio testing and local compilation were not completed in this shell because:

- No C/C++ compiler is available on PATH.
- No Ninja or Make build tool is available.
- No MSBuild is available on PATH.
- The existing CMake build cache was created from a different Windows path.

The next required validation step is to build on the proper Windows/macOS build machine or GitHub Actions runner and test in a DAW/plugin host.

## Expected User-Facing Result

After these changes:

- Loading the plugin at default settings should not change level or tone.
- Mix at `0%` should be fully dry.
- Mix should control processed amount, not act as an output volume workaround.
- Drive at `0%` should not color the audio.
- Auto Gain should make processed levels easier to manage.
- Saturation modes should sound less harsh and more clipper-like.
- Bass should remain more stable when low-end preservation is active.
- Tape hiss should not appear unless deliberately enabled.
