#!/usr/bin/env bash
set -euo pipefail

readonly LLVM_VERSION="23.1.5"
readonly LLVM_SHA256="c3107384835f788768f614db610e87fba7077f125ca950eb02a78bab0a308065"

command -v zstd >/dev/null 2>&1 || {
    printf 'ERROR: zstd is required to extract the official LLVM release.\n' >&2
    exit 1
}

archive="$(mktemp)"
file_list="$(mktemp)"
trap 'rm -f -- "${archive}" "${file_list}"' EXIT
if [[ -n "${LLVM_ARCHIVE:-}" ]]; then
    cp -- "${LLVM_ARCHIVE}" "${archive}"
else
    curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 --connect-timeout 30 --max-time 600 \
        "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.zst" \
        -o "${archive}"
fi
printf '%s  %s\n' "${LLVM_SHA256}" "${archive}" | sha256sum --check

install_root="${HOME}/.local/opt"
mkdir -p "${install_root}"
archive_root="$(zstd --long=30 -dc "${archive}" | tar -tf - | sed -n '1{s#/.*##;p;}')"
[[ -n "${archive_root}" && "${archive_root}" != */* ]] || {
    printf 'ERROR: the LLVM release archive has an unexpected layout.\n' >&2
    exit 1
}
zstd --long=30 -dc "${archive}" | tar -tf - |
    grep -E "^${archive_root}/(bin/(clang|clang\\+\\+|clangd|clang-tidy|clang-format)(-[0-9]+)?|lib/lib(clang|LLVM)[^/]*\\.so[^/]*|lib/clang/23/)" \
        | grep -v '/$' \
        > "${file_list}"
zstd --long=30 -dc "${archive}" | tar -xf - -C "${install_root}" -T "${file_list}"

llvm_bin="${install_root}/${archive_root}/bin"
local_bin="${HOME}/.local/bin"
mkdir -p "${local_bin}"
for tool in clang clang++ clangd clang-format clang-tidy; do
    [[ -x "${llvm_bin}/${tool}" ]] || {
        printf 'ERROR: the official LLVM archive is missing %s.\n' "${tool}" >&2
        exit 1
    }
    target="${local_bin}/${tool}-23"
    expected="$(readlink -f "${llvm_bin}/${tool}")"
    if [[ -e "${target}" || -L "${target}" ]]; then
        [[ "$(readlink -f "${target}")" == "${expected}" ]] || {
            printf 'ERROR: refusing to replace existing %s at %s.\n' "${tool}-23" "${target}" >&2
            exit 1
        }
    else
        ln -s "${llvm_bin}/${tool}" "${target}"
    fi
done

export PATH="${local_bin}:${PATH}"
if [[ -n "${GITHUB_PATH:-}" ]]; then
    printf '%s\n' "${local_bin}" >> "${GITHUB_PATH}"
fi

clang-23 --version | grep -F "${LLVM_VERSION}"
clang++-23 --version | grep -F "${LLVM_VERSION}"
clang-format-23 --version | grep -F "${LLVM_VERSION}"
clang-tidy-23 --version | grep -F "${LLVM_VERSION}"
clangd-23 --version | grep -F "${LLVM_VERSION}"
