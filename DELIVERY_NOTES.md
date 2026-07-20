# WapDem Saturation Delivery Notes

## What To Send

Send the installer for the platform first:

- `WapDemSaturation-windows-installer.exe` for Windows.
- `WapDemSaturation-macos-installer.pkg` for macOS.

Also keep the raw zip as a backup/manual install package:

- `WapDemSaturation-windows.zip` for Windows.
- `WapDemSaturation-macos.zip` for macOS.
- Use the AAX zip only when the AAX SDK build and signing flow were completed.

Do not send both the zip and the extracted folder as separate deliverables. They contain the same plugin files and make the package look duplicated.

## What "Installer App" Means

An installer app is a normal setup program that copies the plugin files to the correct system folders automatically.

For this project:

- Windows installer: `.exe` made with Inno Setup.
- macOS installer: `.pkg` made with `pkgbuild`.
- Windows VST3 installs to `C:\Program Files\Common Files\VST3\`.
- macOS VST3 installs to `/Library/Audio/Plug-Ins/VST3/`.
- macOS AU installs to `/Library/Audio/Plug-Ins/Components/`.

## Windows Security Warning

The standalone `.exe` and plugin binaries are unsigned unless a code-signing certificate is applied after the build.

Unsigned audio-plugin binaries downloaded from the internet can trigger Windows SmartScreen or antivirus warnings even when the build is clean. This is a trust/signing issue, not an installer feature.

For public release, use proper signing:

- Sign the Windows `.exe` and plugin binaries with a trusted Authenticode certificate.
- Notarize/sign macOS builds with an Apple Developer ID.
- Sign AAX builds with the required PACE/Avid process.

## Install Locations

Windows VST3:

```text
C:\Program Files\Common Files\VST3\
```

Windows AAX:

```text
C:\Program Files\Common Files\Avid\Audio\Plug-Ins\
```

macOS VST3:

```text
/Library/Audio/Plug-Ins/VST3/
```

macOS AU:

```text
/Library/Audio/Plug-Ins/Components/
```

macOS AAX:

```text
/Library/Application Support/Avid/Audio/Plug-Ins/
```
