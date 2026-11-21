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
11. Interactive Windows validation.
12. Production release acceptance.

## Non-negotiable constraints

- C++20.
- Windows 10/11 x64.
- MSVC cl.exe/MSBuild/CMake/VS 2022/v143/Windows SDK.
- No clang-cl, MinGW, MSYS2 or CLANG64 production fallback.
- No custom desktop UI or candidate popup.
- MIT and clean-room constraints.
