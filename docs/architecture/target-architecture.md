# SUBORNO target architecture and repository assessment

## Decision summary

Keep one shared, platform-neutral C++20 engine and native operating-system adapters. Keep Windows 10/11 x64 with MSVC v143 and TSF as the first production platform. Treat Linux (IBus/Fcitx 5), macOS (InputMethodKit), and Android as later adapters, not as compile-only placeholder targets.

The current include/suborno + src layout is a sound starting point. Do not perform a mass directory move before the dependency graph is repaired: moving files now would create churn without fixing the actual architectural defects. Refactor module boundaries and CMake targets first, then move files in small verified slices.

## Reference implementations

| Project | Verified strengths | Structural / product limitations relevant to SUBORNO |
|---|---|---|
| BornoTSF | Native Windows TSF server; separately named composition, core, display, infrastructure, layout parsers, and storage areas; Borno/Avro/Khipro/National/Probhat feature breadth is a useful Windows compatibility checklist. | The implementation is Windows-specific and its current repository is GPL-3.0. Use public behavior and UI/compatibility observations only under the clean-room policy; do not port implementation code or tables. |
| Avro Keyboard | Mature phonetic workflow, autocorrect, dictionary/spell-checking, layout editor, skin/layout tooling, and a modular Delphi application tree. The upstream repository is MPL-2.0. | Its desktop application is not a portable engine + per-platform adapter architecture. MPL source files and mapping assets have source-file obligations; they are not automatically suitable for an MIT clean-room rewrite. |
| Bongo | Shared Rust phonetic engine exposed via C FFI; offline lexicon; distinct macOS InputMethodKit, Linux IBus/Fcitx 5, and Windows service/application trees; release and platform validation documentation. | Its documented Windows frontend uses a low-level keyboard hook and SendInput rather than Windows TSF. That is not the Windows integration model selected for SUBORNO. Its architecture is a useful reference for cross-platform separation, not a design to copy wholesale. |

Reference repositories and documents:
- BornoTSF: https://github.com/JayedAhsan/BornoTSF
- Avro Keyboard: https://github.com/mugli/Avro-Keyboard
- Bongo architecture: https://github.com/mehedishakeel/Bongo/blob/main/docs/ARCHITECTURE.md
- Bongo validation: https://github.com/mehedishakeel/Bongo/blob/main/docs/VALIDATION.md

## Current repository assessment

### What is already in the right direction

- Public headers are grouped under include/suborno, implementation files under src, tests under tests, and platform-specific Windows implementation under src/platform/windows.
- The generic InputLayout / InputSession contract is the right abstraction boundary for independent layout engines and per-context mutable composition state.
- Windows TSF is kept out of the portable C++ targets and is built as a native COM in-process server.
- Unicode handling is centralized behind an ICU-backed module.
- Settings persistence, dictionary access, candidate generation, spell checking, tests, and documentation have their own areas.
- Build and quality tooling is kept outside the typing hot path.

### Production blockers found by source inspection

1. **C ABI was coupled to a concrete phonetic session.** The public session factory returned std::unique_ptr<layout::InputSession>, but the C wrapper stored std::unique_ptr<layouts::PhoneticInputSession>. This broke GCC, Clang, and MSVC builds and contradicted the generic layout API. The wrapper has now been corrected to own the base interface, and a C ABI regression test has been added.
2. **National and Probhat are not yet verified fixed physical layouts.** The current national_layout.cpp is a Roman-token transliterator. A class name and LayoutKind do not make a keymap compliant with the National standard. The current implementation must remain marked incomplete until an authoritative key-position corpus and physical-key tests exist. Apply the same evidence requirement to Probhat.
3. **Key identity is not yet physical enough.** Windows TSF currently converts virtual-key values using a US-ASCII mapping. A production fixed layout needs stable physical key identity (scan code / extended-key state or a normalized HID-position model) separately from the character produced by the active Windows keyboard layout. Phonetic input must consume the correct logical text, while fixed layouts consume physical positions.
4. **The C ABI is incomplete as a stable public SDK.** It has no explicit ABI-versioned configuration/session options, layout selection, key-event entry point, length-aware append function, or structured error details. These must be designed before third-party clients depend on it.
5. **Candidate, phonetic, and dictionary responsibilities overlap.** Candidate collection/ranking must be a shared service with a layout-provided primary candidate and optional dictionary/user/learning sources. A phonetic-only helper must not become the generic ranking layer.
6. **The desktop UI is not complete.** The Windows app is currently a small Win32 control window and tray integration. Candidate popup/UIElement, real layout switch UX, Topbar, settings panels, keyboard viewer, accessibility, and DPI behavior are not yet delivered.
7. **Settings are more complete as persistence than as runtime behavior.** Every persisted setting needs an explicit consumer, UI binding, validation rule, migration behavior, and tests; otherwise it should not appear as a working product control.
8. **Linux and macOS adapters currently report unavailable.** They are placeholders, not supported platform implementations. They must not be advertised or packaged as working ports.
9. **The current CMake root file owns too much target construction.** It is workable at this size, but it should be decomposed by module only after public/private include boundaries and target dependencies are stable.
10. **Production verification is incomplete.** Hosted compile/test jobs and a custom ICU-data POC are not equivalent to interactive typing tests, upgrade/uninstall validation, or a signed release on a clean Windows machine.

## Target module boundaries

Keep the existing repository root and migrate the internals toward these logical modules:

    include/suborno/
      api/                 # versioned C ABI and public SDK headers
      base/                # Result, Error, IDs, limits, shared value types
      unicode/             # UTF-8, normalization, grapheme utilities
      input/               # logical/physical key event contracts
      layout-api/          # layout definitions, sessions, capabilities
      engine/              # engine configuration, registry, session factory
      layouts/
        phonetic/          # parser and phonetic layout implementations
        fixed/             # National, Probhat, other physical keymaps
        compositional/     # Khipro-like stateful orthographic composition
      language/
        dictionary/        # lexicon interfaces and storage contracts
        candidates/        # collection, ranking, selection, learning
        spell/             # spelling and autocorrect services
      settings/            # versioned settings types and store interface

    src/
      base/
      unicode/
      input/
      layout-api/
      engine/
      layouts/{phonetic,fixed,compositional}/
      language/{dictionary,candidates,spell,learning}/
      settings/
      persistence/
      platform/
        windows/{tsf,com,registration,app,ui}/
        linux/{ibus,fcitx5}/
        macos/{input-method-kit}/

    tests/
      unit/
      integration/
      linguistic/{borno,avro,national,probhat,khipro}/
      system/windows/
      fuzz/
      fixtures/
      benchmarks/

    installer/windows/
    assets/
    docs/{architecture,decisions,specifications,testing,research}/
    cmake/modules/

This is a logical target tree, not an instruction to move every current file immediately. Public API headers must remain minimal; Windows COM/TSF implementation headers should be private to the Windows adapter unless a real supported consumer needs them.

## Dependency direction

The production dependency graph must be acyclic and point inward:

    Windows TSF / Windows UI
              |
              v
          Platform API
              |
              v
       Engine / Context Runtime
          /      |       \
         v       v        v
      Layouts  Candidates Settings
         |        |         |
         v        v         v
      Layout API Dictionary Persistence
          \        |       /
           v       v      v
             Unicode / Base

Rules:

- Layout implementations depend on the layout API, Unicode utilities, and narrowly scoped language-service interfaces; they must not depend on Windows, UI, file paths, or global settings.
- A session owns mutable state for one input context. Layout definitions are immutable and shareable.
- Platform adapters translate host events and composition operations. They do not implement phonetic rules, keymaps, candidate ranking, or dictionary policy.
- The engine wires built-in services. Layouts must not construct SQLite connections or access the filesystem directly.
- Candidate collection and ranking are separate from the parser. A layout supplies its primary output and optional alternatives; dictionary and learning sources contribute candidates through interfaces.
- User interface, telemetry (disabled by default), update checks, voice, and sync never execute in the synchronous keystroke path.
- No arbitrary DLL/plugin loading in the first stable release. Start with a trusted built-in registry and versioned declarative layout data. Add third-party plugin loading only after a stable ABI and signing model exist.

## Input event and composition contract

Do not conflate physical keys with typed characters. The normalized event must preserve at least:

- physical position identity (scan code / HID usage, extended-key bit, and key location);
- logical virtual key;
- text generated by the active keyboard layout, when available;
- Shift, Control, Alt, Windows, Caps Lock, Num Lock, and AltGr state;
- key-down / key-up and repeat information.

Phonetic engines consume logical text and profile-specific control-key information. Fixed layouts consume physical position and modifier state. Compositional layouts consume a sequence of normalized editing commands and may own a richer state machine.

Every context session must support: insert, backspace by grapheme cluster, delete/left/right as appropriate, commit, cancel, candidate navigation/selection, focus loss, host composition termination, and bounded-input failure without corrupting already committed text.

## Candidate and language-intelligence pipeline

1. Layout parser produces primary preedit and deterministic alternatives.
2. Candidate collector queries cached local lexicons and the user dictionary through an explicit interface.
3. Ranker combines source priority, frequency, context, user corrections, and stable tie-breaking.
4. Candidate selector updates the preedit and commits the selected candidate.
5. Learning is local, opt-in, bounded, and resilient to corrupted storage.

The first shipping milestone must work fully offline. Any optional language model is a separately packaged, measurable enhancement with no network dependency and no unbounded latency.

## Windows-first production scope

Release 1 is Windows 10/11 x64 only:

- MSVC cl.exe, MSBuild, Visual Studio 2022 generator, v143, Windows SDK.
- TSF in-process COM text service, per-context composition state, correct edit-session lifecycle, display attributes, preserved key handling, profile registration, and clean deactivation.
- Native Win32 UI for settings/tray/candidate selection, correct DPI scaling, keyboard accessibility, and screen-reader labels.
- Per-user install and uninstall, upgrade over previous builds, rollback on failed install, no stale COM registration, and signed installer/binaries.
- Real typing verification across Notepad, Word/Office, browsers, VS Code, terminals, and elevated/non-elevated application boundaries.

Linux/Fcitx5, Linux/IBus, and macOS/InputMethodKit become separate release milestones after the Windows engine contract is stable and exercised by shared conformance tests.

## Production acceptance gates

A release cannot be called production-ready until every gate is evidenced:

| Gate | Required evidence |
|---|---|
| Build | Clean Release and Debug builds on supported Windows 10/11 x64 with MSVC v143; reproducible configure/build instructions. |
| Tests | Unit, integration, linguistic corpus, property, fuzz smoke, and Windows TSF contract tests pass. No skipped tests caused by failed compilation. |
| Linguistic compatibility | Versioned input/output corpus for Suborno, Borno regular behavior, Avro behavior, National, Probhat, and Khipro. Every unsupported case is explicit. |
| Unicode | Malformed UTF-8, NFC behavior, Bengali conjuncts, nukta forms, ZWJ/ZWNJ, EGC backspace, emoji, cursor and selection edge cases. |
| Host compatibility | Interactive test matrix across supported Windows applications and input contexts. |
| Reliability | No crash/hang on focus switching, app close during composition, TSF deactivate, corrupt settings/database, long input, or rapid layout switching. |
| Performance | Measured cold start, steady-state key-to-preedit p50/p95/p99 latency, memory, candidate query cost, and idle CPU on low-end hardware. Targets are set from baseline measurements, not guessed. |
| Security/privacy | Offline-by-default verification, no input-content telemetry, safe path handling, bounded parser inputs, dependency/license inventory, and security review. |
| Installer | Install/upgrade/uninstall/rollback tests on clean Windows images, per-user permissions, correct registration, and no orphaned files/registry keys. |
| Release | Signed artifacts, SBOM/third-party notices, changelog, reproducible release workflow, recovery instructions, and documented support scope. |

## Implementation order

1. Restore a clean cross-compiler build and keep the C ABI layout-agnostic.
2. Freeze layout API v1 and define logical-vs-physical key events.
3. Implement real physical National and Probhat maps using independently verified published key-position corpora.
4. Extract candidate collection/ranking into a shared language service and remove phonetic-specific generic dependencies.
5. Complete Avro and Borno compatibility against versioned clean-room corpora; add Khipro as an independent compositional engine.
6. Finish Windows TSF lifecycle, candidate UIElement/window, display attributes, per-context switching, settings consumption, and native UX.
7. Complete dictionary, autocorrect, personal vocabulary, learning controls, and corruption recovery.
8. Add interactive Windows host matrix, fuzz/regression automation, and measured performance baselines.
9. Complete installer/signing/upgrade/uninstall and production release acceptance.
10. Start Linux/macOS adapters only after the shared conformance suite passes.

No feature may be marked complete solely because a type, source file, placeholder adapter, or settings field exists. Completion requires behavior, tests, integration, documentation, and platform evidence.
