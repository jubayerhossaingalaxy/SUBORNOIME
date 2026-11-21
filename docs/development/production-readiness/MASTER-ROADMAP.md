# SUBORNO Production Readiness Master Roadmap

## Status

- Product status: not production ready.
- Current repository status: portable Bangla phonetic engine, initial Khipro compositional subset, Windows COM/TSF text service, and installer source.
- Windows TSF/COM status: implementation and contract tests are present; hosted MSVC validation and interactive typing verification are pending.

## Phases

1. Complete repository audit.
2. Core engine and public contracts.
3. Layout API and registry.
4. Independent composition sessions.
5. Dictionary and candidate integration.
6. Settings and persistence.
7. Windows COM/TSF integration.
8. Linguistic, fuzz and reliability quality.
9. Performance baseline.
10. Installer and signing.
11. Borno and Avro phonetic profiles plus Khipro composition.
12. Physical National keymap (blocked on BDS 1738:2018 licensing clearance); Probhat mapping and keyboard event normalization.
13. Candidate UIElement and candidate-window behavior.
14. Versioned General/Interface/Topbar/Input desktop settings.
15. Native Topbar, themes, on-screen keyboard, and layout studio.
16. Unicode/ANSI compatibility and conversion tooling.
17. SyncType companion, pairing, and voice provider.
18. Interactive Windows validation.
19. Production release acceptance.



## Audited implementation snapshot — 2026-11-21

The following implementation slices now exist in the current PR branch. They
remain subject to the CI and acceptance evidence listed in the parity matrix.

- **Probhat:** fixed physical scan-code Base/Shift/Caps layers now follow the
  referenced MIT KLC table; context-sensitive vowel rewriting was removed.
  AltGr columns and KLC dead-key state transitions remain incomplete.
- **Candidate UI:** a COM `ITfCandidateListUIElement` implementation and
  native non-activating vertical popup exist. TSF count/string/selection/page
  contracts have Windows unit coverage. Real host-app rendering, caret behavior,
  keyboard interaction and per-monitor DPI behavior still need acceptance.
- **Learning:** committed words and committed candidates increment bounded
  personal usage frequency and feed dictionary ranking. This is not a full
  language model or explicit choice-event/negative-feedback system.
- **Settings/session refresh:** idle TSF contexts can adopt a changed layout
  after the in-flight composition ends; an active composition is preserved
  until it is committed or cancelled.
- **Dictionary:** normalized prefix lookup is bounded at the SQLite query
  boundary and merged into candidates with duplicate suppression.

## Current release blockers

- Latest PR-head CI must pass Clang 23 formatting/tidy/tests/build, GCC 13
  tests/build, and MSVC v143 Release/Debug plus Windows contract tests.
- Minimal ICU data POC must produce reproducible size measurements and pass
  tests before any production ICU-data change is accepted.
- AltGr and dead-key compatibility for Probhat is incomplete.
- Borno LM, Avro full autocorrect/context parity, Khipro full parity and the
  National physical map are not complete.
- Full dictionary corpus provenance, spell-correction integration, next-word
  prediction and language-quality evaluation remain outstanding.
- Candidate UI requires live TSF host-app validation, per-monitor DPI work and
  keyboard/accessibility review.
- Topbar, settings UX, themes, on-screen keyboard, layout studio, ANSI/Bijoy,
  SyncType, Android companion and voice remain unimplemented or partial.
- The installer is unsigned; checksum verification is not code signing.

## Non-negotiable constraints

- C++20.
- Windows 10/11 x64.
- MSVC cl.exe/MSBuild/CMake/VS 2022/v143/Windows SDK.
- No clang-cl, MinGW, MSYS2 or CLANG64 production fallback.
- Native Win32 desktop UI only; no Electron/Tauri shell.
- Candidate popup, Topbar and settings must be native Windows components.
- Voice/companion services are optional and must not enter the typing hot path.
- MIT and clean-room constraints.
