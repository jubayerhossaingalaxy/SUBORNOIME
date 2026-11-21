# SUBORNO Production Readiness Master Roadmap

## Status

- Product status: not production ready.
- Current repository status: portable Bangla phonetic engine, Windows COM/TSF text service, and installer source.
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
12. Physical National/Probhat keymaps and keyboard event normalization.
13. Candidate UIElement and candidate-window behavior.
14. Versioned General/Interface/Topbar/Input desktop settings.
15. Native Topbar, themes, on-screen keyboard, and layout studio.
16. Unicode/ANSI compatibility and conversion tooling.
17. SyncType companion, pairing, and voice provider.
18. Interactive Windows validation.
19. Production release acceptance.

## Non-negotiable constraints

- C++20.
- Windows 10/11 x64.
- MSVC cl.exe/MSBuild/CMake/VS 2022/v143/Windows SDK.
- No clang-cl, MinGW, MSYS2 or CLANG64 production fallback.
- Native Win32 desktop UI only; no Electron/Tauri shell.
- Candidate popup, Topbar and settings must be native Windows components.
- Voice/companion services are optional and must not enter the typing hot path.
- MIT and clean-room constraints.
