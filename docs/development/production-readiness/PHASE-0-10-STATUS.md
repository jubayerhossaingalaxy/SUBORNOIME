# SUBORNOIME — Phase 0–10 Execution Status

**Last updated:** 2026-11-21  
**Tracked branch:** `optimize/remove-unused-icu-i18n`  
**Release status:** NOT PRODUCTION READY

This tracker records implementation evidence separately from release acceptance.
A phase is not complete merely because source code or a test exists. It requires
current-head CI evidence and the applicable real-world acceptance checks.

| Phase | Scope | Status | Remaining acceptance work |
|---|---|---|---|
| 0 | Repository baseline, audit, scope, license and risk inventory | Partial | Complete source/build/security/license audit; reconcile all docs with the actual source and approved product scope |
| 1 | Portable C++20 core, API contracts, module boundaries and registry | Implemented foundation; CI pending | Current-head GCC/Clang/MSVC builds, ABI/API review, lifecycle and concurrency tests |
| 2 | Phonetic engine and Avro/Borno behavior | Partial | Golden corpora; Avro autocorrect/context parity; Borno LM/context parity; escaping, backspace and composition edge cases |
| 3 | Independent keyboard layouts | Partial | Probhat dead-key behavior and full physical-position validation; National physical map remains blocked on licensing clearance; Khipro full parity. Probhat Base/Shift/AltGr/Shift+AltGr and CapsLock flags are implemented; CI pending |
| 4 | Bengali Unicode and text composition | Partial | Current-head tests and full corpus acceptance; grapheme/ZWJ/ZWNJ boundary validation and composition edge cases remain. Unicode 18 assigned Bengali-block inventory and categories are implemented; Assamese-specific U+09F0/U+09F1 are excluded |
| 5 | Dictionary, candidates and language intelligence | Partial | Current-head tests; dictionary corpus provenance/license; language-quality evaluation; spell integration; prediction and learning controls |
| 6 | Windows COM/TSF integration | Implementation present; CI and interactive acceptance pending | MSVC Release/Debug; TSF lifecycle, focus, edit-session and candidate behavior in Notepad, Office and browsers |
| 7 | Native desktop UI, settings and persistence | Partial | Full settings UX, accessibility, DPI/multi-monitor behavior, layout controls and corruption recovery acceptance |
| 8 | Security, reliability, performance and compatibility | Partial | Current-head sanitizer/fuzz/security/performance evidence; race/lifetime review; supported Windows host matrix |
| 9 | Installer, upgrade, rollback, signing and release engineering | Partial | Clean-machine install/upgrade/uninstall; x86/x64 architecture policy; signed binaries and verified checksums |
| 10 | Final production acceptance | Blocked | All previous gates pass; release artifact, provenance, SBOM, license audit, signed installer and final manual test report |

## Current-head changes awaiting CI

- Dictionary rejects a future schema version before applying migrations; regression
  coverage verifies the database schema remains unchanged.
- Dictionary filesystem paths are interpreted as UTF-8 and passed to SQLite as
  UTF-8; a Bengali filename regression test was added.
- TSF keeps last-known-good settings if a settings reload fails; engine regression
  coverage verifies that the active layout and settings remain intact.
- Avro ya-phala composition now ends the dependent-consonant run correctly so
  the documented `bybohar` vector produces `ব্যবহার`; a regression test was added.
- Probhat KLC Ctrl+Alt and Shift+Ctrl+Alt output layers now use the Windows Right-Alt
  state, and CAPLOKALTGR is applied only within those layers; regression tests added.
- Unicode 18 assigned Bengali-block inventory and General_Category metadata were
  expanded; reserved and Assamese-specific U+09F0/U+09F1 code points are excluded.
- ICU i18n is explicitly linked because grapheme segmentation uses
  `icu::BreakIterator`; the minimal-data POC must retain the required character
  break rules and pass the full Unicode regression suite.
- The Minimal ICU POC workflow cancels stale runs for the same ref.

## Current blockers

1. Latest-head CI has not yet produced a passing result.
2. Minimal ICU data POC has not yet produced reproducible size and test evidence.
3. Avro/Borno/Khipro phonetic parity is incomplete.
4. Probhat dead-key behavior is incomplete; Base/Shift/AltGr/Shift+AltGr and CapsLock
   output layers have implementation and regression coverage, pending current-head CI.
5. National physical keymap is blocked pending licensing clearance for BDS 1738:2018.
6. Dictionary corpus provenance and language-quality evaluation are incomplete.
7. TSF candidate UI has not been validated interactively in real Windows host apps.
8. Full native settings/topbar/theme/on-screen-keyboard/layout-studio scope is incomplete.
9. Installer signing and clean-machine lifecycle acceptance are incomplete.
10. Linux/macOS adapters are contracts only; native platform integration is not complete.

## Execution rules

- Keep changes on the feature branch until all acceptance gates pass.
- Do not claim parity from implementation presence or a small smoke corpus.
- Do not copy GPL/MPL source or unlicensed keymaps, dictionaries, models or assets.
- Do not mark a test as passed until the exact commit SHA has a completed passing run.
- Preserve user data during migrations, upgrades, rollback and uninstall.
- Record unavailable physical-device/host tests as blocked, not passed.
