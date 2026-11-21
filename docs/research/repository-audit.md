# Initial Repository Audit

**Audit date:** 2026-11-17

## Initial state

The workspace was empty: it contained no tracked files, source code, tests,
documentation, build configuration, package manifests, or CI workflows. The
directory was not initialized as a Git repository. Consequently, there were no
existing project conventions or features to preserve and no existing license
file to overwrite.

## Detected development environment

- Ubuntu 24.04.5 LTS, x86_64
- Git 2.56.0
- CMake 3.28.3
- GCC/G++ 13.3.0
- Clang 18.1.3
- Python 3.14.2
- GitHub CLI 2.103.3
- ICU development headers 74.2 and SQLite development headers 3.45.1
- Ninja, clangd, clang-format, clang-tidy, vcpkg, and Grype were not initially
  on `PATH`

The system packages are useful host tools but are not substitutes for the
project's vcpkg-managed dependency graph. Windows SDK, MSVC, COM, and TSF are
not available in this Ubuntu environment.

## Research findings and decisions

- The Microsoft vcpkg registry baseline selected for this initial setup is
  `0df93a05b9de080129e653ec4776d7e14a1b2e4f` (2026-11-16).
- The registry at this baseline lists ICU 79.1, SQLite 3.54.1, WIL
  1.0.260126.7, GoogleTest 1.18.0, and Google Benchmark 1.10.0.
- ICU upstream's current stable release is ICU 79.1, released 2026-10-22;
  therefore the vcpkg registry version and upstream release match. Ubuntu's
  preinstalled ICU 74.2 headers are not the version the CMake build resolves.
- GoogleTest and Google Benchmark are isolated in the optional `development`
  vcpkg feature. WIL is conditional on Windows. The portable core does not
  depend on WIL.
- CMake presets use schema version 6, supported by the detected CMake 3.28.3.
- The Windows target is a native shared-library build boundary only. It does
  not implement COM registration, a TSF text service, or keyboard input.

## Scope and preservation

The scaffold will define portable target boundaries, a real infrastructure
assertion, Windows build-only DLL source, build/test presets, editor and code
quality settings, Ubuntu setup scripts, CI, security scanning, and developer
documentation. No product-level IME behavior is included.

## Official research sources

- Microsoft TSF: <https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework>
- Microsoft COM objects and interfaces: <https://learn.microsoft.com/en-us/windows/win32/com/com-objects-and-interfaces>
- MSVC `/analyze`: <https://learn.microsoft.com/en-us/cpp/build/reference/analyze-code-analysis?view=msvc-170>
- MSVC C++ language standard option: <https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-170>
- Visual Studio CMake presets: <https://learn.microsoft.com/en-us/cpp/build/cmake-presets-vs?view=msvc-170>
- vcpkg manifest mode: <https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode>
- vcpkg CMake integration: <https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration>
- vcpkg baseline update command: <https://learn.microsoft.com/en-us/vcpkg/commands/update-baseline>
- vcpkg registry: <https://github.com/microsoft/vcpkg>
- CMake presets reference: <https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html>
- CMake compilation database export: <https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html>
- ICU4C: <https://unicode-org.github.io/icu/userguide/icu4c/>
- ICU releases: <https://github.com/unicode-org/icu/releases>
- SQLite license: <https://www.sqlite.org/copyright.html>
- GoogleTest: <https://github.com/google/googletest>
- Google Benchmark: <https://github.com/google/benchmark>
- WIL: <https://github.com/microsoft/wil>
- clangd install: <https://clangd.llvm.org/installation.html>
- clangd configuration: <https://clangd.llvm.org/config>
- clang-format: <https://clang.llvm.org/docs/ClangFormat.html>
- clang-tidy: <https://clang.llvm.org/extra/clang-tidy/>
- GitHub-hosted runners: <https://docs.github.com/en/actions/concepts/runners/github-hosted-runners>
- Runner image maintenance: <https://github.com/actions/runner-images>
- Ubuntu 24.04 runner image inventory: <https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2404-Readme.md>
- Windows 2022 runner image inventory: <https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md>
- GitHub Actions workflow syntax: <https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax>
- Syft SPDX generation: <https://oss.anchore.com/docs/guides/sbom/formats/>
- Grype SBOM scanning: <https://github.com/anchore/grype#using-grype>
