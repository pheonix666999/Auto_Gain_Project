#!/usr/bin/env bash
set -euo pipefail

package_dir="${1:-build/dist/WapDemSaturation-macos}"
output_dir="${2:-build/dist}"
pkg_name="${3:-WapDemSaturation-macos-installer.pkg}"
version="${4:-1.0.0}"

if [[ ! -d "${package_dir}" ]]; then
    echo "Package directory not found: ${package_dir}" >&2
    exit 1
fi

root_dir="${output_dir}/pkgroot"
scripts_dir="${output_dir}/pkgscripts"
pkg_path="${output_dir}/${pkg_name}"

rm -rf "${root_dir}" "${scripts_dir}" "${pkg_path}"
mkdir -p "${root_dir}" "${scripts_dir}" "${output_dir}"

copy_bundle_dir() {
    local source_dir="$1"
    local dest_dir="$2"

    if [[ -d "${source_dir}" ]]; then
        mkdir -p "${dest_dir}"
        cp -R "${source_dir}/." "${dest_dir}/"
    fi
}

copy_bundle_dir "${package_dir}/VST3" "${root_dir}/Library/Audio/Plug-Ins/VST3"
copy_bundle_dir "${package_dir}/AU" "${root_dir}/Library/Audio/Plug-Ins/Components"
copy_bundle_dir "${package_dir}/AAX" "${root_dir}/Library/Application Support/Avid/Audio/Plug-Ins"
copy_bundle_dir "${package_dir}/Standalone" "${root_dir}/Applications"

if [[ ! -d "${root_dir}/Library/Audio/Plug-Ins/VST3" \
      && ! -d "${root_dir}/Library/Audio/Plug-Ins/Components" \
      && ! -d "${root_dir}/Library/Application Support/Avid/Audio/Plug-Ins" \
      && ! -d "${root_dir}/Applications" ]]; then
    echo "No macOS installable artefacts found under ${package_dir}" >&2
    exit 1
fi

cat > "${scripts_dir}/postinstall" <<'POSTINSTALL'
#!/bin/sh
set -e

if command -v xattr >/dev/null 2>&1; then
    xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/WapDem Saturation.vst3" 2>/dev/null || true
    xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/WapDem Saturation.component" 2>/dev/null || true
    xattr -dr com.apple.quarantine "/Applications/WapDem Saturation.app" 2>/dev/null || true
fi

exit 0
POSTINSTALL

chmod +x "${scripts_dir}/postinstall"

pkgbuild \
    --root "${root_dir}" \
    --scripts "${scripts_dir}" \
    --identifier "com.tractoneaudio.wapdemsaturation.pkg" \
    --version "${version}" \
    --install-location "/" \
    "${pkg_path}"

echo "Created installer: ${pkg_path}"
