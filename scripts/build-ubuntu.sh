#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
preset="${1:-ubuntu-dev}"
jobs="${SUBORNO_JOBS:-1}"

[[ "${jobs}" =~ ^[1-9][0-9]*$ ]] || {
    printf 'ERROR: SUBORNO_JOBS must be a positive integer; got %s.\n' "${jobs}" >&2
    exit 1
}
cd "${PROJECT_ROOT}"
cmake --build --preset "${preset}" --parallel "${jobs}"
