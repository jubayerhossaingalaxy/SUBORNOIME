#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
build_dir="${1:-${PROJECT_ROOT}/build/ubuntu-dev}"
tidy="${CLANG_TIDY:-clang-tidy-23}"

command -v "${tidy}" >/dev/null 2>&1 || {
    printf 'ERROR: %s is required. Run scripts/bootstrap-ubuntu.sh.\n' "${tidy}" >&2
    exit 1
}
[[ -f "${build_dir}/compile_commands.json" ]] || {
    printf 'ERROR: %s/compile_commands.json is missing. Configure the Ubuntu preset first.\n' \
        "${build_dir}" >&2
    exit 1
}

mapfile -t project_sources < <(
    python3 - "${build_dir}/compile_commands.json" "${PROJECT_ROOT}" <<'PY'
import json
import pathlib
import sys

compile_commands_path = pathlib.Path(sys.argv[1])
project_root = pathlib.Path(sys.argv[2]).resolve()
with compile_commands_path.open(encoding="utf-8") as compile_commands_file:
    compile_commands = json.load(compile_commands_file)

source_files = set()
for command in compile_commands:
    source_path = pathlib.Path(command["file"])
    if not source_path.is_absolute():
        source_path = pathlib.Path(command["directory"]) / source_path

    source_path = source_path.resolve()
    try:
        source_path.relative_to(project_root)
    except ValueError:
        continue

    if source_path.suffix.lower() in {".c", ".cc", ".cpp", ".cxx"}:
        source_files.add(source_path)

for source_path in sorted(source_files):
    print(source_path)
PY
)

(("${#project_sources[@]}" > 0)) || {
    printf 'ERROR: no project source files were found in %s/compile_commands.json.\n' \
        "${build_dir}" >&2
    exit 1
}

"${tidy}" -p "${build_dir}" "${project_sources[@]}"
