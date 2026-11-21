# Architecture and Build Boundaries

## Current status

The repository contains a portable C++20 engine, a generic InputLayout and
InputSession contract, Suborno and initial Borno phonetic profiles, Probhat and
National layout implementations, versioned settings persistence, SQLite
dictionary/candidate integration, and a native Windows COM/TSF text service.
The generic session contract separates phonetic parsing from stateless layout
sessions; stateful compositional layouts can provide their own session.
Interactive desktop typing and clean-machine installation remain outside
automated verification.

## Platform boundaries

- `suborno_core` is a portable C++20 static library.
- `suborno_layout_api` defines generic input layouts, key events, candidates,
  and host-neutral input sessions. The phonetic parser remains a specialized
  implementation of that contract.
- `suborno_layout_sessions` provides the stateless layout-session factory;
  shared candidate generation merges layout and dictionary results.
- `suborno_unicode` carries ICU link requirements. `suborno_dictionary`
  owns SQLite queries and user-word storage. `suborno_persistence` provides
  atomic file replacement and does not require SQLite.
- `suborno_settings` serializes and validates versioned settings through the
  persistence module.
- `src/platform/windows/` is excluded from non-Windows configurations.
  `suborno_windows_tsf` is a native COM in-process server and TSF text service.
  It advises keyboard/thread-manager sinks, manages per-context composition,
  uses TSF edit sessions for composition updates and commits, and exposes the
  initial native candidate-list UIElement/popup. Its live host-app behavior
  remains outside automated acceptance.
- WIL is only found and linked for the Windows target. Ubuntu portable targets
  cannot include Windows SDK, COM, TSF, or WIL headers.
- Windows production configurations fail unless CMake selects MSVC toolset
  v143. No MinGW, GCC, clang-cl, or Zig fallback is permitted.

Future platform adapters should be separate targets that depend on the
platform-neutral API, not on Windows implementation details. Linux, macOS, and
Android adapters are outside the current target.

## Build and dependency graph

```text
suborno_core
  ├── suborno_unicode ── ICU4C
  ├── suborno_layout_api
  ├── suborno_dictionary ── SQLite
  ├── suborno_layout_sessions
  ├── suborno_settings
  └── suborno_persistence (atomic file I/O)

suborno_windows_tsf (Windows only, shared library)
  ├── suborno_core
  └── WIL

suborno_unit_tests / suborno_integration_tests
  └── GoogleTest (development feature)

suborno_benchmarks (optional)
  └── Google Benchmark (development feature)
```

`BUILD_TESTING` controls GoogleTest targets. `SUBORNO_BUILD_BENCHMARKS` is off
by default. CMake target-scoped options enforce C++20 and warning policies.
Ubuntu presets use Ninja and export `compile_commands.json`; Visual Studio
generators use their supported configuration mechanism and do not rely on that
database.

## Verification boundaries

Ubuntu CI exercises the portable core and tests with Clang and GCC. Windows CI
builds the x64 DLL and tests using MSVC v143 and the Windows SDK. Hosted Windows checks build the DLL and installer and run non-interactive
COM/TSF contract tests. Actual desktop typing and application compatibility
need a separate interactive Windows test environment.
