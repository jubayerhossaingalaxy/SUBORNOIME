#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT}"

"${SCRIPT_DIR}/configure-ubuntu.sh" ubuntu-dev
"${SCRIPT_DIR}/build-ubuntu.sh" ubuntu-dev
"${SCRIPT_DIR}/test-ubuntu.sh" ubuntu-dev
"${SCRIPT_DIR}/check-format-ubuntu.sh"
"${SCRIPT_DIR}/check-tidy-ubuntu.sh"
