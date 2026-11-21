# Suborno Repository Instructions

## Project Identity

- Brand: Suborno
- Product: Suborno
- Technical project: subornoime
- Shared C++20 engine with independent layout modules and platform adapters
- First production platform: Windows 10 x64
- Desktop UI is outside scope

## Compiler and Build Constraints

- Windows production uses MSVC `cl.exe`, MSBuild, CMake, Visual Studio 17 2022,
  v143 and the Windows SDK for x64.
- Do not replace or add clang-cl, MinGW, MSYS2, GCC or CLANG64 for Windows
  production.
- Ubuntu with Clang or GCC is portability validation only.
- Preserve CMake presets and `/m:1` low-memory build behavior.

## Source Rules

- Preserve existing working code and public behavior.
- Keep portable Engine code independent of COM, HRESULT and Windows headers.
- Keep Windows TSF/COM code under `src/platform/windows/`.
- Reuse existing modules, tests and CMake targets before creating new ones.
- Use RAII and explicit ownership. Do not add owning raw pointers.
- Keep the MIT license objective and clean-room constraints.

## Validation Rules

- Use `scripts/verify-ubuntu.sh` for portable verification when practical.
- Use the Ubuntu build/test scripts for targeted checks.
- Never claim a test passed unless it was executed and observed.
- Never claim Windows TSF or desktop compatibility from compilation alone.
- Run focused tests before broader suites.

## Change Rules

- Do not change source, tests, CMake or CI unless the task explicitly approves
  those changes.
- Do not commit, push or delete branches without explicit approval.
- Do not install dependencies or execute untrusted scripts without approval.
- Do not add custom settings UI, tray UI, dashboard or candidate popup.
