#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
build_dir="${1:-${PROJECT_ROOT}/build/ubuntu-dev}"
installed_dir="${build_dir}/vcpkg_installed"
sbom_dir="${build_dir}/spdx"
report="${build_dir}/grype-report.txt"

for tool in syft grype; do
    command -v "${tool}" >/dev/null 2>&1 || {
        printf 'ERROR: %s is required. Run scripts/bootstrap-ubuntu.sh.\n' "${tool}" >&2
        exit 1
    }
done
[[ -d "${installed_dir}" ]] || {
    printf 'ERROR: vcpkg installation was not found at %s. Configure the selected preset first.\n' \
        "${installed_dir}" >&2
    exit 1
}

mapfile -t vcpkg_sboms < <(
    find "${installed_dir}" -type f -path '*/share/*/vcpkg.spdx.json' -print | sort
)
((${#vcpkg_sboms[@]})) || {
    printf 'ERROR: vcpkg did not generate any per-package SPDX SBOM files in %s.\n' \
        "${installed_dir}" >&2
    exit 1
}

mkdir -p "${sbom_dir}"
: >"${report}"
for vcpkg_sbom in "${vcpkg_sboms[@]}"; do
    port_dir="$(dirname -- "${vcpkg_sbom}")"
    port_name="$(basename -- "${port_dir}")"
    sbom="${sbom_dir}/${port_name}.spdx.json"

    syft "dir:${port_dir}" \
        --select-catalogers +sbom-cataloger \
        --source-name "vcpkg-${port_name}" \
        --source-version 0.1.0 \
        -o "spdx-json=${sbom}"
    {
        printf '\n## %s\n' "${port_name}"
        grype "sbom:${sbom}" -o table
    } >>"${report}"
done

cat "${report}"
printf '\nSPDX SBOMs: %s/*.spdx.json\nGrype report: %s\n' "${sbom_dir}" "${report}"
