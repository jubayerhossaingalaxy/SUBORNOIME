# SUBORNO Production Readiness Progress

## Current Windows TSF work

### Implemented

- Implemented the Windows COM class factory and DLL registration entry points.
- Added TSF language-profile/category registration and unregistration.
- Added per-context phonetic key handling, TSF composition edit sessions, commit, candidate digit selection, backspace, and cancellation.
- Added COM lifetime and Windows contract tests, plus installer compilation in the Windows CI job.
- Fixed atomic settings persistence and added focused replacement tests.

### Verified locally

- Configured with the pinned vcpkg packages using the available Clang 18 toolchain.
- Ran local validation in the requested order: format, lint, test targets/CTest (2/2 suites), then full build; all passed.
- Hosted CI run 44766128685 passed with Clang 23 format/lint/tests/full build, GCC 13 tests/full build, Windows MSVC v143 Release/Debug contract tests/full builds, and installer compilation.
- A local Windows SDK/MSVC toolchain is unavailable; interactive TSF typing and clean-machine installer verification remain pending.

## Historical baseline snapshot

### Completed

- Re-established repository baseline.
- Verified branch `main`.
- Re-verified current SUBORNO source and build targets.
- Re-verified the 7 project Skills and 9 custom agents.
- Re-verified Microsoft Learn MCP read-only endpoint.
- Created the production-readiness tracking directory.
- Provisioned the pinned vcpkg baseline.
- Verified GCC 13.3.0 native C++20 compilation.
- Built and passed the Ubuntu unit/integration baseline: 2/2 tests.
- Verified formatting and existing clang-tidy execution.
- Implemented portable Engine orchestration and LayoutRegistry.
- Added layout-injected composition sessions.
- Integrated exact SQLite dictionary matches into CandidateEngine.
- Made Dictionary PIMPL non-copyable and RAII-owned.
- Passed GCC 13 CTest: 2/2 suites, including 43 unit tests and integration tests.
- Added portable versioned SettingsStore with atomic save, load, defaults and malformed/unsupported file tests.
- Re-ran `scripts/verify-ubuntu.sh`; configure/build/CTest/format/tidy completed with exit status 0.
- Added settings/persistence tests and a linguistic regression corpus.
- Built and ran ASan/UBSan CTest: 2/2 suites passed.
- Built libFuzzer target and completed 1,000 runs without crash or sanitizer finding.
- Measured bounded phonetic benchmarks: transliteration 4.2 µs, candidate generation 37.1 µs.
- Built Ubuntu CI Clang preset and passed 2/2 CTest suites.
- Generated SPDX SBOM and ran Grype: no vulnerabilities found for installed dependencies.
- Added corrupt-database, old-schema migration and prefix/reset dictionary tests.
- Added public C ABI, declarative layout loader, deterministic language intelligence
  and Linux/macOS platform adapter contracts.
- At the time of this snapshot, Windows COM/TSF implementation was still blocked
  by the lack of a local Windows SDK/MSVC environment; the implementation is
  recorded in the current-work section above.

### In progress

- Provision the pinned Ubuntu vcpkg baseline and local verification tools.
- Run the existing portable build/test/format/tidy suite.

### Blocked

- Interactive Windows TSF/COM typing verification remains unavailable until a
  Windows desktop host is used.

## Evidence policy

No pass claim is recorded without an executed command and observed result.



## Additional implementation slices — 2026-11-21

- Repaired a dictionary compile issue and moved the stateless phonetic session
  factory into the phonetic-session translation unit.
- Added bounded normalized-prefix dictionary candidates with deduplication,
  lower ranking than exact matches, and candidate-engine tests.
- Added commit-frequency persistence for personal words, including system
  entries learned through commits; new words become visible in personal
  dictionary queries and usage frequency affects ranking.
- Replaced Probhat's earlier context-dependent vowel heuristic with a physical
  scan-code Base/Shift/Caps mapping derived from the MIT-licensed KLC reference.
  Added attribution and a mapping specification. AltGr and dead-key composition
  are explicitly not yet supported.
- Added TSF per-context session-layout tracking so settings changes are adopted
  by idle contexts without replacing an in-flight composition.
- Added native candidate popup module-instance resolution using the module
  containing the window procedure, rather than assuming the EXE module handle.
- Updated source-backed feature statuses; implementation presence is not
  treated as release acceptance.

