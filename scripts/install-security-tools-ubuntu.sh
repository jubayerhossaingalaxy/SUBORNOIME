#!/usr/bin/env bash
set -euo pipefail

readonly SYFT_VERSION="1.57.1"
readonly SYFT_SHA256="bf52519eb57f675d47c07d43213cb6dff6449f3d6d1147ff6701ed91f44eef0c"
readonly GRYPE_VERSION="0.121.3"
readonly GRYPE_SHA256="e9da18cc1429e786722e32161288103600efffd8e483c90761f90485bd1bec85"

readonly BIN_DIR="${HOME}/.local/bin"
tmp_dir="$(mktemp -d)"
trap 'rm -rf -- "${tmp_dir}"' EXIT
mkdir -p "${BIN_DIR}"

curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 --connect-timeout 30 --max-time 600 \
    "https://github.com/anchore/syft/releases/download/v${SYFT_VERSION}/syft_${SYFT_VERSION}_linux_amd64.tar.gz" \
    -o "${tmp_dir}/syft.tar.gz"
printf '%s  %s\n' "${SYFT_SHA256}" "${tmp_dir}/syft.tar.gz" | sha256sum --check
tar -xzf "${tmp_dir}/syft.tar.gz" -C "${tmp_dir}" syft
install -m 755 "${tmp_dir}/syft" "${BIN_DIR}/syft"

curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 --connect-timeout 30 --max-time 600 \
    "https://github.com/anchore/grype/releases/download/v${GRYPE_VERSION}/grype_${GRYPE_VERSION}_linux_amd64.tar.gz" \
    -o "${tmp_dir}/grype.tar.gz"
printf '%s  %s\n' "${GRYPE_SHA256}" "${tmp_dir}/grype.tar.gz" | sha256sum --check
tar -xzf "${tmp_dir}/grype.tar.gz" -C "${tmp_dir}" grype
install -m 755 "${tmp_dir}/grype" "${BIN_DIR}/grype"

export PATH="${BIN_DIR}:${PATH}"
syft version | grep -F "${SYFT_VERSION}"
grype version | grep -F "${GRYPE_VERSION}"

if [[ -n "${GITHUB_PATH:-}" ]]; then
    printf '%s\n' "${BIN_DIR}" >> "${GITHUB_PATH}"
fi
