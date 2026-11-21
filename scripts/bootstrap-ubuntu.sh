#!/usr/bin/env bash
set -euo pipefail

readonly REQUIRED_UBUNTU="24.04"
readonly VCPKG_BASELINE="0df93a05b9de080129e653ec4776d7e14a1b2e4f"
readonly VCPKG_DEFAULT_ROOT="${HOME}/.cache/suborno/vcpkg"

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

[[ -r /etc/os-release ]] || fail "Cannot identify the operating system (/etc/os-release missing)."
# shellcheck disable=SC1091
source /etc/os-release
[[ "${ID:-}" == ubuntu ]] || fail "This bootstrap supports Ubuntu ${REQUIRED_UBUNTU}; detected ${ID:-unknown}."
[[ "${VERSION_ID:-}" == "${REQUIRED_UBUNTU}" ]] ||
    fail "Ubuntu ${REQUIRED_UBUNTU} is required; detected ${VERSION_ID:-unknown}."
[[ "$(uname -m)" == x86_64 ]] || fail "Ubuntu x86_64 is required; detected $(uname -m)."
command -v sudo >/dev/null 2>&1 || fail "sudo is required to install missing Ubuntu packages."
command -v apt-get >/dev/null 2>&1 || fail "apt-get was not found."

declare -A package_commands=(
    [build-essential]="g++"
    [libgmp-dev]="pkg-config"
    [libmpfr-dev]="pkg-config"
    [libmpc-dev]="pkg-config"
    [cmake]="cmake"
    [ninja-build]="ninja"
    [git]="git"
    [curl]="curl"
    [zip]="zip"
    [unzip]="unzip"
    [tar]="tar"
    [zstd]="zstd"
    [pkg-config]="pkg-config"
    [python3]="python3"
)

missing_packages=()
for package in "${!package_commands[@]}"; do
    if ! dpkg-query -W -f='${Status}' "${package}" 2>/dev/null | grep -q 'install ok installed'; then
        missing_packages+=("${package}")
    fi
done

if ((${#missing_packages[@]})); then
    printf 'Installing missing Ubuntu packages: %s\n' "${missing_packages[*]}"
    sudo apt-get update
    sudo apt-get install -y --no-install-recommends "${missing_packages[@]}"
else
    printf 'All required Ubuntu packages are already installed.\n'
fi

scripts/install-llvm-23-ubuntu.sh
scripts/install-linux-build-tools.sh
export PATH="${HOME}/.local/bin:${PATH}"

for package in "${!package_commands[@]}"; do
    tool="${package_commands[$package]}"
    command -v "${tool}" >/dev/null 2>&1 || fail "Package ${package} did not provide required tool ${tool}."
done

export PATH="${HOME}/.local/bin:${PATH}"
if ! command -v grype >/dev/null 2>&1; then
    command -v curl >/dev/null 2>&1 || fail "curl is required to install Grype from Anchore's official installer."
    mkdir -p "${HOME}/.local/bin"
    curl -sSfL https://get.anchore.io/grype | sh -s -- -b "${HOME}/.local/bin"
fi
command -v grype >/dev/null 2>&1 || fail "Grype installation failed; see https://oss.anchore.com/docs/installation/grype/."

if ! command -v syft >/dev/null 2>&1; then
    command -v curl >/dev/null 2>&1 || fail "curl is required to install Syft from Anchore's official installer."
    mkdir -p "${HOME}/.local/bin"
    curl -sSfL https://get.anchore.io/syft | sh -s -- -b "${HOME}/.local/bin"
fi
command -v syft >/dev/null 2>&1 || fail "Syft installation failed; see https://oss.anchore.com/docs/installation/syft/."

VCPKG_ROOT="${VCPKG_ROOT:-${VCPKG_DEFAULT_ROOT}}"
if [[ -x "${VCPKG_ROOT}/vcpkg" ]]; then
    actual_baseline="$(git -C "${VCPKG_ROOT}" rev-parse HEAD 2>/dev/null || true)"
    [[ "${actual_baseline}" == "${VCPKG_BASELINE}" ]] ||
        fail "Existing VCPKG_ROOT is at ${actual_baseline:-unknown}, not the required baseline ${VCPKG_BASELINE}. Set VCPKG_ROOT to a separate checkout; this script will not rewrite an existing checkout."
else
    [[ ! -e "${VCPKG_ROOT}" ]] ||
        fail "${VCPKG_ROOT} exists but is not a usable vcpkg checkout. Move it manually or select another VCPKG_ROOT."
    mkdir -p "$(dirname "${VCPKG_ROOT}")"
    git clone --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}"
    git -C "${VCPKG_ROOT}" fetch --depth 1 origin "${VCPKG_BASELINE}"
    git -C "${VCPKG_ROOT}" checkout --detach "${VCPKG_BASELINE}"
    "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
fi

[[ -x "${VCPKG_ROOT}/vcpkg" ]] || fail "vcpkg executable was not created at ${VCPKG_ROOT}/vcpkg."
printf '\nBootstrap complete.\n'
printf 'Ubuntu: %s (%s)\n' "${VERSION_ID}" "$(uname -m)"
for tool in git cmake ninja g++-13 clang++-23 clangd-23 clang-format-23 clang-tidy-23 python3 syft grype; do
    printf '%-18s %s\n' "${tool}" "$("${tool}" --version 2>&1 | head -n 1)"
done
printf 'vcpkg baseline: %s\n' "$(git -C "${VCPKG_ROOT}" rev-parse HEAD)"
printf '\nFor this terminal, run:\n  export VCPKG_ROOT="%s"\n' "${VCPKG_ROOT}"
