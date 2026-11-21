# Suborno (subornoime)

**Suborno** is the Windows application; **subornoime** is the technical
project identifier.

subornoime is a portable C++20 Bangla phonetic engine with a native Windows
Text Services Framework (TSF) adapter. The repository includes a phonetic
parser, layout registry, independent composition sessions, SQLite-backed
dictionary, candidate ranking, settings persistence, and COM/TSF registration
and edit-session code. Windows CI and interactive typing still require
verification before this can be considered production ready.

## Scope and platform

- Production target: Windows 10/11 x64.
- Windows integration direction: native MSVC v143, Win32, COM, and Text Services
  Framework (TSF).
- Ubuntu 24.04 is the primary development environment for the platform-neutral
  C++20 code.
- The source and target boundaries keep the portable core separate from the
  Windows adapter. Linux, macOS, and Android input method adapters are not
  implemented or claimed as supported.
- The Windows targets include the Suborno desktop application and an in-process
  COM text service. The application provides layout selection and Windows input
  profile controls; interactive typing and application compatibility are not
  covered by hosted CI.

See [the architecture overview](docs/architecture/overview.md) and
[development setup](docs/development/ubuntu.md) for details.

## Technology

- C++20 and CMake with CMake Presets
- Ubuntu: Clang 23, GCC 13, Ninja, clangd, clang-format, and clang-tidy
- Windows production build: MSVC v143, Windows SDK, MSBuild, and the Visual
  Studio 17 2022 CMake generator
- vcpkg manifest mode with a pinned registry baseline
- ICU4C and SQLite as production dependencies; WIL on Windows only
- GoogleTest and Google Benchmark as optional development dependencies

## Prerequisites

Ubuntu 24.04 x86_64, Git, and `sudo` for installing missing system packages.
Run `scripts/bootstrap-ubuntu.sh` to detect and install missing tools and set
up the pinned vcpkg checkout. Windows compilation is performed by the
`windows-2022` GitHub Actions runner; Ubuntu is not expected to provide a
Windows SDK or MSVC.

## Build and test on Ubuntu

```bash
scripts/bootstrap-ubuntu.sh
export PATH="$HOME/.local/bin:$PATH"
export VCPKG_ROOT="$HOME/.cache/suborno/vcpkg"
scripts/configure-ubuntu.sh
scripts/build-ubuntu.sh
scripts/test-ubuntu.sh
scripts/check-format-ubuntu.sh
scripts/check-tidy-ubuntu.sh
```

By default, builds use one job. Set `SUBORNO_JOBS` to a positive integer to
override this for `scripts/build-ubuntu.sh`. Benchmarks are disabled in normal
development builds; see [Ubuntu development instructions](docs/development/ubuntu.md)
to enable the optional target.

## Windows build

The consolidated GitHub Actions CI workflow builds and tests with Clang 23 and
GCC 13 on Ubuntu 24.04 and MSVC v143 on Windows 2022 (Release and Debug), and
compiles the Windows installer. Version tags trigger the separate Windows
release workflow. Hosted tests cover DLL/COM activation contracts and
registration routines, but not live typing in desktop applications. See
[Windows build instructions](docs/development/windows.md).

## Tests

GoogleTest covers phonetic parsing and composition, supported layout mappings,
Bengali Unicode handling, dictionary and settings behavior, the public C API,
and portable integration contracts. Windows tests cover DLL loading, COM
factory/service activation, server lifetime, and TSF registration calls. They
do not verify interactive typing or application compatibility. CTest is used
on Ubuntu and Windows. The optional benchmark is only a build/configuration
smoke target and does not represent IME performance.

## Security and dependencies

Dependency versions, licenses, and source URLs are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Security reporting is
described in [SECURITY.md](SECURITY.md). No signing certificates or credentials
belong in this repository.
