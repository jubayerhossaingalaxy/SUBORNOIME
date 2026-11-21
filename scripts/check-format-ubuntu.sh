#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
formatter="${CLANG_FORMAT:-clang-format-23}"

command -v "${formatter}" >/dev/null 2>&1 || {
    printf 'ERROR: %s is required. Run scripts/bootstrap-ubuntu.sh.\n' "${formatter}" >&2
    exit 1
}
mapfile -d '' sources < <(
    find "${PROJECT_ROOT}/include" "${PROJECT_ROOT}/src" "${PROJECT_ROOT}/tests" \
        "${PROJECT_ROOT}/benchmarks" -type f \
        \( -name '*.h' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) \
        -print0
)
((${#sources[@]})) || { printf 'ERROR: no C++ sources were found.\n' >&2; exit 1; }
"${formatter}" --dry-run --Werror "${sources[@]}"
