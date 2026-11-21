# Suborno

**A native Bangla input method for Windows, built with modern C++.**

Suborno brings Bangla typing tools together in one project: a phonetic composition engine, multiple keyboard layouts, Unicode-aware text handling, dictionary-backed candidates, and a native Windows Text Services Framework (TSF) integration.

The application is named **Suborno**. The repository and technical project identifier is **`subornoime`**.

> **Project status:** Active development. The Windows x64 application, engine, and TSF integration are implemented in the repository. Automated builds and tests cover important code and COM/TSF contracts, but interactive typing in everyday desktop applications and clean-machine installation still need manual verification.

---

## What Suborno includes

- **Bangla phonetic typing** — a case-sensitive Roman-token parser with composition state and ranked alternatives.
- **Keyboard layouts** — phonetic, Probhat, and National layout implementations.
- **Unicode-aware text handling** — strict UTF-8 validation, NFC/NFKC normalization, and extended grapheme-cluster handling.
- **Dictionary and candidates** — SQLite-backed dictionary storage and candidate generation/ranking infrastructure.
- **Windows integration** — a native C++ desktop application and an in-process COM/TSF text-service DLL.
- **Settings and persistence** — user settings and file persistence support.

These describe the code and components in this repository; they are not a claim that every interactive Windows workflow has already been validated.

## Platform support

| Platform | Project status |
|---|---|
| **Windows 10/11 x64** | Primary application and production target. Uses MSVC v143, Win32, COM, and TSF. |
| **Ubuntu 24.04 x86_64** | Development and CI platform for the portable C++ engine and tests. It is not currently a supported Linux input method. |
| **Linux desktop IME, macOS, Android** | Not currently supported as user-facing input methods. |

The core is kept separate from platform-specific integration so that future adapters can be developed without coupling the engine to Windows APIs.

## Getting started

### Build on Windows

A local Windows build requires:

- Windows 10/11 x64
- Visual Studio 2022 or Build Tools with the **MSVC v143** toolset
- A Windows SDK
- CMake 3.25 or newer
- Git
- A vcpkg checkout at the baseline pinned in `vcpkg.json`

Open a Visual Studio Developer PowerShell or a terminal with the MSVC build environment available, then run these commands from the repository root.

```powershell
# Point this to your existing vcpkg checkout.
$env:VCPKG_ROOT = "D:\path\to\vcpkg"

# Configure the Windows x64 build.
cmake --preset windows-msvc

# Build the Release application, TSF service, and configured targets.
cmake --build --preset windows-msvc-release --parallel 1

# Run the Release test suite.
ctest --preset windows-msvc-release --output-on-failure
```

The Windows production build intentionally requires **MSVC v143**. MinGW, GCC, MSYS2, and clang-cl are not supported for this target.

For installer, registration, and manual verification details, see the [Windows build and verification guide](docs/development/windows.md).

### Build the portable engine on Ubuntu

Ubuntu 24.04 is used to develop and validate the platform-neutral C++ code. The bootstrap script installs or prepares the required tools and pinned vcpkg checkout.

```bash
# One-time setup
scripts/bootstrap-ubuntu.sh
export PATH="$HOME/.local/bin:$PATH"
export VCPKG_ROOT="$HOME/.cache/suborno/vcpkg"

# Configure, build, and test
scripts/configure-ubuntu.sh
scripts/build-ubuntu.sh
scripts/test-ubuntu.sh

# Optional quality checks
scripts/check-format-ubuntu.sh
scripts/check-tidy-ubuntu.sh
```

Build scripts use one job by default to keep memory use modest. To choose a different job count for the build script, set `SUBORNO_JOBS` to a positive integer.

See the [Ubuntu development guide](docs/development/ubuntu.md) for setup details and optional tooling.

## Tests and verification

The project uses CMake Presets and CTest.

Automated tests cover areas including:

- Phonetic parsing, composition, and candidate alternatives
- Probhat and National layout mappings
- Bengali Unicode validation, normalization, and grapheme handling
- Dictionary, settings, and persistence behavior
- Portable engine and integration contracts
- Windows DLL loading, COM activation contracts, and TSF registration routines

**What still needs hands-on verification:** live typing through Windows TSF in applications such as Notepad, compatibility with other desktop applications, and installation/uninstallation on a clean Windows machine. Passing a build or a non-interactive test does not prove those scenarios work.

You can follow the current automation in [GitHub Actions](https://github.com/jubayerhossaingalaxy/SUBORNOIME/actions).

## Project structure

| Path | Purpose |
|---|---|
| `include/suborno/` | Public C++ headers and API |
| `src/core/` | Shared engine and public C API |
| `src/unicode/` | Bengali and Unicode utilities |
| `src/layouts/` | Phonetic, Probhat, and National layouts |
| `src/dictionary/`, `src/candidates/` | Dictionary and candidate infrastructure |
| `src/platform/windows/` | Windows application, COM, and TSF adapter |
| `tests/` | Unit, integration, and Windows contract tests |
| `cmake/`, `scripts/` | Build configuration and developer tools |
| `docs/` | Architecture, specifications, and development guides |

For a deeper explanation, start with the [architecture overview](docs/architecture/overview.md).

## Contributing

Contributions and thoughtful bug reports are welcome. Before changing behavior, please read the relevant architecture and specification documents so that changes remain consistent with the engine's platform boundaries and phonetic rules.

- [Contribution guide](CONTRIBUTING.md)
- [Architecture overview](docs/architecture/overview.md)
- [Bengali Unicode specification](docs/specifications/bengali-unicode-support.md)
- [Phonetic mapping specification](docs/specifications/phonetic-mapping-spec.md)
- [Security policy](SECURITY.md)

Please include the steps to reproduce a bug and the expected versus actual behavior. For code changes, run the relevant tests and checks where your development environment supports them.

## Dependencies and license

Suborno is released under the [MIT License](LICENSE).

The project uses ICU4C and SQLite for core functionality, WIL for Windows integration, and GoogleTest / Google Benchmark for optional development and testing. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency versions, licenses, and upstream links.

---

**Thanks for taking a look at Suborno.** The goal is to build a dependable, native Bangla typing experience while keeping the text engine modular, testable, and Unicode-aware.
