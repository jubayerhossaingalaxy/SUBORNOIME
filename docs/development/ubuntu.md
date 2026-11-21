# Ubuntu 24.04 Development Setup

The supported development host is Ubuntu 24.04 LTS x86_64. The bootstrap
script checks the OS and architecture, installs LLVM/Clang 23 from the official
stable release artifact, uses the existing Ubuntu GCC 13 toolchain, installs
CMake 4.5.0 and Ninja 1.13.2 from verified upstream
release artifacts, installs Syft and Grype using Anchore's official installers,
and creates a vcpkg checkout at the pinned baseline. It does not install
redundant apt copies of project libraries: ICU, SQLite, GoogleTest, and Google
Benchmark are resolved with vcpkg.

## One-time setup

```bash
scripts/bootstrap-ubuntu.sh
export PATH="$HOME/.local/bin:$PATH"
export VCPKG_ROOT="$HOME/.cache/suborno/vcpkg"
```

The bootstrap's vcpkg checkout is isolated from other checkouts and will not
rewrite an existing directory at another revision. If vcpkg is already
installed elsewhere, set `VCPKG_ROOT` to a separate checkout at the baseline in
`vcpkg.json`; `scripts/configure-ubuntu.sh` verifies the exact commit.

## Updating the vcpkg baseline

Baseline changes intentionally require review. In a separate vcpkg clone,
fetch the registry revision you intend to use, check out that revision, and
from the repository root inspect the proposed manifest changes before
committing:

```bash
export VCPKG_UPDATE_ROOT="$HOME/.cache/suborno/vcpkg-update"
git clone --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git "$VCPKG_UPDATE_ROOT"
git -C "$VCPKG_UPDATE_ROOT" fetch --depth 1 origin master
git -C "$VCPKG_UPDATE_ROOT" checkout --detach FETCH_HEAD
"$VCPKG_UPDATE_ROOT/bootstrap-vcpkg.sh" -disableMetrics
"$VCPKG_UPDATE_ROOT/vcpkg" x-update-baseline --dry-run
"$VCPKG_UPDATE_ROOT/vcpkg" x-update-baseline
```

Run the `x-update-baseline` commands from the subornoime repository root.
The vcpkg-documented `x-update-baseline` command updates the manifest's
configured registry baselines; it is experimental. Review the changed
`builtin-baseline` and all resolved package versions, licenses, and platform
support, then commit the baseline and any deliberate version constraints
together. Do not copy a moving branch name into `builtin-baseline`; the command
records a commit. Keep a separate pinned checkout for normal development rather
than changing the bootstrap-managed baseline in place.

## Configure, build, and test

```bash
scripts/configure-ubuntu.sh ubuntu-dev
scripts/build-ubuntu.sh ubuntu-dev
scripts/test-ubuntu.sh ubuntu-dev
```

Equivalent direct commands, after exporting `VCPKG_ROOT`:

```bash
cmake --preset ubuntu-dev
cmake --build --preset ubuntu-dev --parallel 1
ctest --preset ubuntu-dev
```

The build preset uses one job for resource-constrained computers. To explicitly
override the script's job count:

```bash
SUBORNO_JOBS=2 scripts/build-ubuntu.sh ubuntu-dev
```

Ninja creates `build/ubuntu-dev/compile_commands.json`, consumed by clangd and
clang-tidy. The development preset explicitly uses `clang++-23`; the GCC CI
preset explicitly uses `g++-13` (GCC 13). GCC CI checks the configured
compiler and every entry in `build/ubuntu-gcc-ci/compile_commands.json` after
configuration, without compiling sources. These checks verify compiler
selection, not successful compilation or runtime behavior; those require the
subsequent build and test steps. Normal development disables benchmark
compilation.

## Optional benchmark target

The `benchmarks` manifest feature is intentionally not part of the default
Ubuntu development preset. Enable it in an isolated build directory:

```bash
cmake -S . -B build/ubuntu-benchmarks -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_CXX_COMPILER=clang++-23 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DVCPKG_MANIFEST_FEATURES='testing;benchmarks' \
  -DSUBORNO_BUILD_BENCHMARKS=ON
cmake --build build/ubuntu-benchmarks --target suborno_benchmarks --parallel 1
```

## Formatting and static analysis

```bash
scripts/check-format-ubuntu.sh
scripts/check-tidy-ubuntu.sh
```

The formatting check uses `clang-format-23` and `.clang-format`. To format an
individual file locally, run:

```bash
clang-format-23 -i path/to/file.cpp
```

`clang-tidy-23` reads the configured compile database. `g++-13` is provided by
Ubuntu 24.04 and remains unmodified. CMake/clangd/clang-tidy
diagnostics in this environment apply to portable sources only; Ubuntu does
not supply Windows SDK, MSVC COM declarations, or TSF headers. Windows-only
sources are checked by Windows CI with MSVC and `/analyze`.

To run clangd from a terminal:

```bash
clangd-23 --background-index --clang-tidy \
  --compile-commands-dir="$PWD/build/ubuntu-dev"
```

VS Code recommends clangd, CMake Tools, EditorConfig, GitHub Actions, and YAML
extensions, and disables the competing Microsoft C/C++ IntelliSense engine.

## Security scanning

The pinned vcpkg installation creates per-package SPDX files beneath
`vcpkg_installed/x64-linux/share/<port>/vcpkg.spdx.json` automatically; its
CLI has no aggregate-SBOM generation subcommand (`vcpkg help` was checked).
Syft reads each package SBOM and emits SPDX JSON, which Grype then scans:

```bash
scripts/security-scan-ubuntu.sh build/ubuntu-dev
```

The command writes one file per installed vcpkg port under
`build/ubuntu-dev/spdx/` and an aggregated text report at
`build/ubuntu-dev/grype-report.txt`. This avoids treating an SBOM for only the
installation directory itself as a complete dependency inventory. Syft
supports SPDX JSON output and Grype documents SPDX SBOMs as a supported input.
Do not scan a stale SBOM and call it a current dependency audit. Grype findings
are informational initially; review confirmed Critical and High items
promptly and document false positives, affected-version evidence, and
remediation decisions. See [SECURITY.md](../../SECURITY.md).
