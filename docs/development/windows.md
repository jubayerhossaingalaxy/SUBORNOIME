# Windows Build and Verification

The only production platform target in this phase is Windows 10/11 x64. The
approved compiler is MSVC `cl.exe` from Visual Studio 2022's v143 toolset,
along with the Windows SDK, MSBuild, CMake, and vcpkg. MinGW, GCC, MSYS2,
clang-cl, and Zig are not supported Windows production compilers.

## Hosted build

`.github/workflows/ci.yml` uses the `windows-2022` GitHub-hosted runner. It
configures `Visual Studio 17 2022`, x64, and v143 via the `windows-msvc` preset.

The vcpkg root is created on the runner at the exact manifest baseline.
Configuration uses vcpkg manifest mode and the `testing` feature. CMake builds
Release and Debug with `/W4`; `/analyze` is applied to production targets, not
GoogleTest executables because their constant-based assertion macros produce
noise such as C6326. It runs non-interactive CTest tests, builds the DLL, and
compiles the installer.
The CI workflow does not publish artifacts. To get an installer for interactive
testing, manually dispatch the `Release` workflow on a branch; it uploads the
installer as a temporary Actions artifact. Pushing a `v*` tag publishes a
GitHub Release.

## Local commands on an appropriately provisioned Windows host

From a Visual Studio 2022 developer environment with v143 and a Windows SDK:

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg-at-the-pinned-baseline"
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release --parallel 1
ctest --preset windows-msvc-release
cmake --build --preset windows-msvc-debug --parallel 1
ctest --preset windows-msvc-debug
```

The CMake target deliberately errors unless the selected compiler uses v143.
Visual Studio's generator is multi-configuration; use the Release and Debug
build/test presets rather than setting `CMAKE_BUILD_TYPE`. The generator does
not emit `compile_commands.json`; Ubuntu clangd is for portable sources.
The Windows preset uses the `x64-windows-static` vcpkg triplet and static MSVC
runtime so ICU, SQLite, and the CRT are not loose application-folder DLLs. The
installer keeps only `Suborno.exe` and Inno Setup's uninstaller pair in Program
Files; it installs shared resources and the TSF COM server under
`%ProgramData%\Suborno`. Settings and user dictionaries are kept under
`%APPDATA%\Suborno`; legacy Local AppData dictionaries are migrated through
SQLite's online backup API and retained at their source. CI audits the
executable imports and verifies all three locations.

## TSF implementation and verification boundary

The DLL implements COM class-factory entry points, registration/unregistration
for its in-process server and Bengali TSF language profile, and per-context
phonetic composition through TSF edit sessions. CI tests module loading,
COM/service creation, server locks, and the registration calls. It also
compiles the Inno Setup installer.

Hosted CI does not type into desktop applications or validate activation,
composition, candidate selection, installer behavior on a clean machine, or
application compatibility. Those require a controlled, interactive Windows
verification session. Release signing and clean-machine verification are also
separate release tasks. See the [Windows 10 build and verification guide](production-readiness/README.md)
for local build, installer, live-typing, and uninstall steps.

The installer provides `Suborno.exe`, a native GUI and notification-area
application with layout selection, TSF profile status, Windows input settings,
and per-user sign-in startup controls. The desktop and Start Menu shortcuts
launch this application; the optional legacy Quick Launch shortcut is also
available. Select Suborno with **Win+Space** to type in other applications.
The GUI's close button hides it to the notification area; exiting the GUI does
not stop TSF instances hosted by other applications.
