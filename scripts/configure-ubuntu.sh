#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
readonly VCPKG_BASELINE="0df93a05b9de080129e653ec4776d7e14a1b2e4f"
export VCPKG_ROOT="${VCPKG_ROOT:-${HOME}/.cache/suborno/vcpkg}"
export VCPKG_MAX_CONCURRENCY="${VCPKG_MAX_CONCURRENCY:-1}"

[[ -x "${VCPKG_ROOT}/vcpkg" ]] || {
    printf 'ERROR: vcpkg is missing at %s. Run scripts/bootstrap-ubuntu.sh first.\n' "${VCPKG_ROOT}" >&2
    exit 1
}
[[ "$(git -C "${VCPKG_ROOT}" rev-parse HEAD)" == "${VCPKG_BASELINE}" ]] || {
    printf 'ERROR: vcpkg checkout does not match the pinned project baseline %s.\n' "${VCPKG_BASELINE}" >&2
    exit 1
}
command -v cmake >/dev/null 2>&1 || { printf 'ERROR: CMake is required.\n' >&2; exit 1; }

cd "${PROJECT_ROOT}"
cmake --preset "${1:-ubuntu-dev}"
