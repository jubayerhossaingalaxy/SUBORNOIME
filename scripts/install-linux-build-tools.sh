#!/usr/bin/env bash
set -euo pipefail

readonly CMAKE_VERSION="4.5.0"
readonly CMAKE_SHA256="67a05954850976717d14ae9bd6fea305a084c650c8eb58c69b8fb92231ee39cb"
readonly NINJA_VERSION="1.13.2"
readonly NINJA_SHA256="5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6"

cmake_installer="$(mktemp)"
ninja_archive="$(mktemp)"
trap 'rm -f -- "${cmake_installer}" "${ninja_archive}"' EXIT

curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 --connect-timeout 30 --max-time 600 \
    "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh" \
    -o "${cmake_installer}"
printf '%s  %s\n' "${CMAKE_SHA256}" "${cmake_installer}" | sha256sum --check

curl -fsSL \
    "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip" \
    -o "${ninja_archive}"
printf '%s  %s\n' "${NINJA_SHA256}" "${ninja_archive}" | sha256sum --check

install_dir="${HOME}/.local"
mkdir -p "${install_dir}/bin"
sh "${cmake_installer}" --skip-license --prefix="${install_dir}"
unzip -p "${ninja_archive}" ninja > "${install_dir}/bin/ninja"
chmod 755 "${install_dir}/bin/ninja"

if [[ -n "${GITHUB_PATH:-}" ]]; then
    printf '%s\n' "${install_dir}/bin" >> "${GITHUB_PATH}"
fi
export PATH="${install_dir}/bin:${PATH}"

cmake --version | grep -F "cmake version ${CMAKE_VERSION}"
[[ "$(ninja --version)" == "${NINJA_VERSION}" ]]
