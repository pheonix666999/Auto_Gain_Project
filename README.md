# WapDem Saturation — Tractone Audio

A harmonic drive / saturation plugin built with **JUCE 8** and **CMake**.
Modes: **Soft Clip · Hard Clip · Tape**. Oversampling: **Off · 2x · 4x**.
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
- CI uploads both the packaged `.zip` and the unpacked delivery folder with a
  `checksums.txt` manifest for large artefacts.
- macOS CI uses the current `macos-15` hosted runner label.

---

## 4. Project layout

```
CMakeLists.txt           Build config + format selection
Source/
  Parameters.h           All param IDs, ranges, choices (APVTS layout)
  PluginProcessor.*       Audio engine: routing, oversampling, mix, meters, presets
  PluginEditor.*          GUI matching the Tractone mockup
  dsp/
    Saturation.h          Soft / Hard / Tape waveshapers
    ToneFilter.h          Single-knob tilt EQ (dark <-> bright)
    LevelMeter.h          Lock-free peak meter source
  gui/
    PluginLookAndFeel.h   Arc knobs, combo styling, palette
    VerticalMeter.h       Gradient IN/OUT meters
```

### Signal flow
```
input ── (dry copy) ───────────────────────────────────┐
   │                                                     │
   └─ [oversample ↑] → saturate → [oversample ↓] → tone ─┤→ parallel mix → output gain → out
                                              (+ hiss in Tape mode)
```

---

## 5. What is implemented

- ✅ Full APVTS parameter set with host automation + state save/recall
- ✅ Three saturation modes with normalised, musical curves
- ✅ `juce::dsp::Oversampling` at 2x / 4x (latency reported to host)
- ✅ Single-knob tilt tone control
- ✅ Parallel dry/wet mix with click-free smoothing (denormals guarded)
- ✅ Tape mode coloration + level-controlled hiss
- ✅ Lock-free IN/OUT peak meters with gradient ballistics
- ✅ Custom LookAndFeel: arc knobs + meters matching the design
- ✅ 6 factory presets with prev/next recall
- ✅ Active + Bypass handling

## 6. Documented TODO / polish (next pass)

- **GUI art assets** — the mockup's background texture, lion artwork, crown
  logo and exact fonts aren't shipped here. Drop them into a `Resources/`
  folder, add via `juce_add_binary_data`, and draw in `PluginEditor::paint`.
  Layout is already pixel-positioned to the 900×600 mockup.
- **User presets** — `savePreset` button is wired but file IO (save/load a
  `.wapdem` preset to the user folder) is stubbed.
- **Undo/redo + gear menu** — header buttons are placeholders.
- **Tape mode** — currently coloration + hiss; optional wow/flutter modulation
  could be added for more character.
- Run **pluginval** (strictness 10) and test in Pro Tools / Logic / Ableton /
  Reaper before release.

---

*Note:* the original prototype was a Cabbage/Csound sketch. This is a clean
JUCE re-implementation using that sketch + the GUI mockup as the spec.
```
