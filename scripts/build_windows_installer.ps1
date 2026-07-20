param(
    [string]$PackageDir = "build\dist\WapDemSaturation-windows",
    [string]$OutputDir = "build\dist",
    [string]$OutputBaseFilename = "WapDemSaturation-windows-installer"
)

$ErrorActionPreference = "Stop"

$isccCommand = Get-Command "iscc.exe" -ErrorAction SilentlyContinue
$isccPath = if ($isccCommand) { $isccCommand.Source } else { $null }

if (-not $isccPath) {
    $defaultPath = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    if (Test-Path $defaultPath) {
        $isccPath = $defaultPath
    }
}

if (-not $isccPath) {
    throw "Inno Setup compiler (ISCC.exe) was not found. Install Inno Setup 6 first."
}

if (-not (Test-Path $PackageDir)) {
    throw "Package directory not found: $PackageDir"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$scriptPath = Join-Path $PSScriptRoot "..\installer\windows\WapDemSaturation.iss"

& $isccPath `
    "/DSourceDir=$((Resolve-Path $PackageDir).Path)" `
    "/DOutputDir=$((Resolve-Path $OutputDir).Path)" `
    "/DOutputBaseFilename=$OutputBaseFilename" `
    $scriptPath

$installerPath = Join-Path $OutputDir "$OutputBaseFilename.exe"
if (-not (Test-Path $installerPath)) {
    throw "Installer was not created: $installerPath"
}

Write-Host "Created installer: $installerPath"
