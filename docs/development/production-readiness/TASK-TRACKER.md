# SUBORNO Task Tracker

| ID | Task | Status | Dependencies | Evidence |
|---|---|---|---|---|
| P0-1 | Complete repository audit | done | none | Source/build/AI ecosystem audit |
| P0-2 | Restore pinned Ubuntu dependencies | done | network/toolchain access | vcpkg baseline 0df93a05 |
| P0-3 | Run exact-head cross-compiler build/tests | in progress | P0-2 | GCC 13 current head: 2/2 CTest pass; Clang 23 and MSVC v143 exact-head validation pending |
| P0-4 | Run formatting and full portable clang-tidy checks | in progress | P0-2 | Earlier Clang format violations were corrected; latest exact-head format/tidy checks pending |
| P1-1 | Central Engine API | done | P0-3 | Engine, layout registry and session factory |
| P1-2 | Layout registry and independent sessions | done | P1-1 | Layout injection and switching tests |
| P3-1 | Dictionary/candidate integration | done | P1-2 | Exact SQLite matches merged and deduplicated |
| P6-1 | Settings and persistence | done | P0-3 | Save/load/default/malformed/unsupported tests pass |
| P3-2 | Dictionary migrations/recovery | done | P3-1 | Corrupt, migration and prefix tests pass |
| P7-1 | Sanitizer and fuzz gates | in progress | P0-3 | ASan/UBSan and 1,000-input libFuzzer smoke added to Clang CI; exact-head evidence pending |
| P8-1 | Performance baseline | partial | P7-1 | Existing core benchmark is not a TSF hot-path, SQLite candidate, first-launch import, RSS or low-end Windows latency benchmark |
| P9-1 | Dependency security scan | in progress | P0-3 | Pinned Syft/Grype SBOM gate and High/Critical failure policy added to Clang CI; exact-head scan pending |
| SDK-1 | C ABI and declarative layouts | done | P1-1 | C API, declarative layout and tests pass |
| LANG-1 | Deterministic language intelligence | done | P3-1 | exact/prefix API and tests pass |
| Linux-1 | Linux adapter contracts | partial | P1-1 | IBus/Fcitx native libraries unavailable |
| macOS-1 | macOS adapter contracts | partial | P1-1 | macOS SDK/host unavailable |
| P5-1 | Windows COM/TSF implementation | done | P1-1/P3-1 | COM server, TSF edit sessions, profile registration, and Windows contract tests added |
| P5-2 | Hosted Windows build and installer validation | in progress | P5-1 | MSVC v143 Debug/Release and Inno Setup CI run pending |
| P5-3 | Interactive Windows typing validation | blocked | P5-2 | Requires desktop Windows session |
| P0-5 | Password/PIN/private-field pass-through | implementation present; interactive acceptance pending | P5-1 | Per-key TSF InputScope read; unknown/failed scopes pass through; Windows unit and host-app tests required |
| P0-6 | Privacy-safe personal learning controls | implementation present; CI pending | P6-1 | Learning OFF by default, desktop opt-in and clear-data action; active composition opt-in is deferred until the next composition |

| LANG-2 | Probhat KLC Base/Shift/Caps physical mapping | implementation present; CI pending | layout-api physical scan-code events | KLC-derived mapping table, literal vowel tests, attribution and spec |
| CAND-1 | Native TSF candidate UIElement / vertical popup | implementation and contract tests present; interactive acceptance pending | P5-1 | Windows count/string/selection/page/GUID tests; live TSF host validation remains |
| CAND-2 | Commit-frequency personalization | implemented foundation; CI pending | P3-2 | Learned-word lifecycle and frequency-ranking regression tests |
| TSF-1 | Refresh idle context session after settings layout change | implementation present; CI pending | P5-1/P6-1 | Session layout marker; preserve in-flight composition; recreate idle session |
| P5-4 | Candidate popup DPI, accessibility and host navigation | not complete | CAND-1/P5-2 | Manual Windows host/app, DPI and keyboard test matrix |
