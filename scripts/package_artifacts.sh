#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo "usage: $0 <build-dir> <platform-name> [config]"
    exit 1
fi

build_dir="$1"
platform_name="$2"
config="${3:-Release}"

artefacts_dir="${build_dir}/WapDemSaturation_artefacts/${config}"
package_name="WapDemSaturation-${platform_name}"
dist_dir="${build_dir}/dist"
package_dir="${dist_dir}/${package_name}"

rm -rf "${package_dir}"
mkdir -p "${package_dir}"

copy_if_exists() {
    local source_path="$1"
    local target_name="$2"

    if [[ -e "${source_path}" ]]; then
        cp -R "${source_path}" "${package_dir}/${target_name}"
    fi
}

copy_if_exists "${artefacts_dir}/Standalone" "Standalone"
copy_if_exists "${artefacts_dir}/VST3" "VST3"
copy_if_exists "${artefacts_dir}/AU" "AU"
copy_if_exists "${artefacts_dir}/AAX" "AAX"

if [[ ! -d "${package_dir}/Standalone" && ! -d "${package_dir}/VST3" && ! -d "${package_dir}/AU" && ! -d "${package_dir}/AAX" ]]; then
    echo "no packaged plugin artefacts found under ${artefacts_dir}"
    exit 1
fi

cp README.md "${package_dir}/README.md"

(
    cd "${package_dir}"
    find . -type f ! -name checksums.txt -print0 | sort -z | xargs -0 shasum -a 256 > checksums.txt
)

(
    cd "${dist_dir}"
    cmake -E tar cf "${package_name}.zip" --format=zip "${package_name}"
)
