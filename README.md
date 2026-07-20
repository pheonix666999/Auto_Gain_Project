# WapDem Saturation — Tractone Audio

A harmonic drive / saturation plugin built with **JUCE 8** and **CMake**.
Saturation types: **Tube · Class A · Tape · Transformer**. Clip modes:
**Soft Clip · Hard Clip**. Oversampling: **Off · 2x · 4x**.
Formats: **VST3 · Standalone** on all supported platforms, **AU** on macOS, optional **AAX**.

---

## 1. Get the source ready

You need JUCE next to this project. Two options:

**Option A — clone JUCE into the project folder (simplest):**
```bash
git clone --branch 8.0.4 https://github.com/juce-framework/JUCE.git
```
The `CMakeLists.txt` auto-detects `./JUCE`.

If you cloned this repository normally and want the pinned JUCE checkout:
```bash
git submodule update --init --recursive
```

**Option B — do nothing.** If `./JUCE` is missing, CMake's `FetchContent`
downloads JUCE 8.0.4 automatically on first configure (needs internet).

---

## 2. Build

### Windows (Visual Studio 2022)
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### macOS (Universal — Intel + Apple Silicon)
```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --config Release
```

Built artefacts are written to `build/WapDemSaturation_artefacts/<Config>/`
because `COPY_PLUGIN_AFTER_BUILD` is off.

---

## 3. AAX (Pro Tools)

AAX is **disabled by default** because it cannot be built or distributed
without Avid's SDK and PACE/iLok signing. To enable:

1. Get the **AAX SDK** from your Avid Developer account.
2. Configure CMake with:
   - `-DWAPDEM_ENABLE_AAX=ON`
   - `-DWAPDEM_AAX_SDK_PATH=/path/to/AAX/SDK`
3. For a build that loads in a normal (protected) Pro Tools install you must
   **sign the .aaxplugin with PACE** (`wraptool`). Unsigned AAX only loads in a
   Pro Tools Developer build.

## 3a. GitHub Actions

- `.github/workflows/build.yml` builds and uploads standard Windows/macOS artefacts.
- `.github/workflows/build-aax.yml` is manual-only and requires an `AAX_SDK_URL`
  secret pointing to a downloadable AAX SDK archive.
- CI uploads the packaged `.zip` plus a platform installer (`.exe` on Windows,
  `.pkg` on macOS). The unpacked duplicate folder is not uploaded.
- CI uses Node 24-compatible action majors and does not initialize submodules;
  if `./JUCE` is absent, CMake fetches the pinned JUCE release.
- macOS CI uses the current `macos-15` hosted runner label.

---

## 4. Project layout

```
CMakeLists.txt           Build config + format selection
Source/
  Parameters.h           All param IDs, ranges, choices (APVTS layout)
  PluginProcessor.*       Audio engine: routing, oversampling, mix, meters, presets
  PluginEditor.*          Compact 860x500 GUI and control wiring
  dsp/
    Saturation.h          Tube / Class A / Tape / Transformer waveshapers
    ToneFilter.h          Single-knob tilt EQ (dark <-> bright)
    LevelMeter.h          Lock-free peak meter source
  gui/
    PluginLookAndFeel.h   Arc knobs, combo styling, palette
    Visualizer.h          Parallel saturation core display
    ControlStripDisplay.h Low-end, harmonics, width/link monitor
    VerticalMeter.h       Gradient IN/OUT meters
```

### Signal flow
```
input ── (dry copy) ───────────────────────────────────┐
   │                                                     │
   └─ low-end split → [oversample ↑] → saturate → [oversample ↓] → tone
      → restore preserved band → punch/width/link → auto-gain ──────┤→ parallel mix → output gain → out
                                                        (+ hiss in Tape mode when enabled)
```

---

## 5. What is implemented

- ✅ Full APVTS parameter set with host automation + state save/recall
- ✅ Four saturation types with normalised, musical curves
- ✅ Separate Soft Clip / Hard Clip modes
- ✅ `juce::dsp::Oversampling` at 2x / 4x (latency reported to host)
- ✅ Single-knob tilt tone control
- ✅ Parallel dry/wet mix with click-free smoothing (denormals guarded)
- ✅ Auto Gain on the wet path before dry/wet mixing
- ✅ Low-end preservation split from 20 Hz to 250 Hz
- ✅ Punch transient contour on the wet path
- ✅ Width and stereo-link processing
- ✅ Tape mode coloration + level-controlled hiss
- ✅ Lock-free IN/OUT peak meters with gradient ballistics
- ✅ Custom LookAndFeel with compact pro layout and live center displays
- ✅ 6 factory presets with prev/next recall
- ✅ Save/load preset buttons, undo/redo buttons, and bypass handling
- ✅ Windows `.exe` installer and macOS `.pkg` installer generated in CI

## 6. Release Notes

- The plugin does not expose a separate Threshold knob. The effective threshold
  is built into the clipper curve; Input, Drive, SAT TYPE, and CLIP MODE control
  how hard the signal hits it.
- Windows and macOS public release builds should be code-signed/notarized to
  reduce SmartScreen, antivirus, and Gatekeeper warnings.
- Run **pluginval** and test in Pro Tools / Logic / Ableton / Reaper before
  sending a public follower release.

---

*Note:* the original prototype was a Cabbage/Csound sketch. This is a clean
JUCE re-implementation using that sketch + the GUI mockup as the spec.
```
