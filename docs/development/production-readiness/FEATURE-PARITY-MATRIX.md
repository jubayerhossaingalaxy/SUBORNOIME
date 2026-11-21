# SUBORNOIME Feature Parity Matrix

This matrix separates implemented code from planned work. A feature is not
complete until its acceptance tests pass and the relevant Windows workflow is
verified.

## Input engine and layouts

| Feature | Status | Acceptance evidence |
|---|---|---|
| Portable C++20 engine | Implemented | Unit and integration tests |
| Generic InputLayout contract | Implemented, CI pending | Engine factory tests |
| Generic InputSession contract | Implemented, CI pending | Stateless session tests |
| Suborno Phonetic | Existing implementation | Phonetic regression corpus |
| Platform-neutral physical key events | Initial implementation, CI pending | key_event_test.cpp and generic session tests |
| Borno Phonetic Regular Mode | Initial implementation, CI pending | borno_phonetic_layout_test.cpp |
| Avro Phonetic profile | Documented conjunct input corpus (299 variants), strict non-conjunct separation and initial vowel-context rules implemented; CI and provenance review pending; not full parity | avro_phonetic_layout_test.cpp, including the 299-vector conjunct corpus |
| Borno Phonetic LM Mode | Planned | Context corpus and ranking evaluation |
| Avro autocorrect and full context rules | Planned | Official-behavior regression corpus |
| Khipro Classic compositional subset | Initial implementation, CI pending; not full parity | Khipro composition, slicer, forced-vowel and separator vectors |
| National physical layout | Not implemented; current module is only a Roman-token prototype. BDS 1738:2018 licensing review required before reproducing the official chart | Written permission / licensing clearance plus an independently authored and tested physical-key matrix |
| Probhat fixed keymap | KLC Base/Shift/AltGr/Shift+AltGr/Caps scan-code layers implemented; OEM_102 and numpad decimal virtual-key paths recognized; unsupported layer combinations pass through; dead-key composition incomplete; CI pending | Physical scan-code, CAPLOK/CAPLOKALTGR, OEM_102, numpad decimal, literal vowel-sign, punctuation and unmapped AltGr tests |
| Phonetic literal escaping | Partial | Single-EGC escape tests |
| Automatic vowel forming | Partial: documented contextual `a` after vowels/signs and `o` after a vowel implemented; full rule behavior pending | Avro vowel-context corpus |
| Old-style/automatic reph | Planned | Reph composition corpus |
| ANSI/Bijoy encoding | Planned | Converter and legacy-app tests |

## Unicode and text handling

| Feature | Status | Acceptance evidence |
|---|---|---|
| Strict UTF-8 validation | Implemented foundation, CI pending | Malformed, overlong, truncated and out-of-range UTF-8 vectors |
| NFC / NFKC normalization | Implemented foundation, CI pending | Bengali nukta canonical decomposition and explicit compatibility-normalization tests |
| Unicode 18 Bengali inventory | Assigned Bengali-block inventory and General_Category metadata implemented; Assamese U+09F0/U+09F1 excluded; CI pending | Inventory uniqueness, category checks, reserved-codepoint and Assamese-exclusion tests |
| Grapheme segmentation | ICU root-locale character BreakIterator; CI pending | UAX #29 combining-mark, Bengali conjunct, ZWJ/ZWNJ, emoji and regional-indicator corpus |
| Host cursor and deletion behavior | Not fully accepted | Interactive TSF editing and grapheme-aware deletion in supported host apps |

## Candidate and language intelligence

| Feature | Status | Acceptance evidence |
|---|---|---|
| Layout primary candidate | Implemented | Layout unit tests |
| Dictionary candidate source | Implemented | Candidate and session tests |
| Candidate-choice learning | Commit-frequency ranking is implemented for committed words/candidates; explicit choice-event analytics, negative feedback and decay are not implemented | Personal-dictionary commit lifecycle and ranking tests; language-quality evaluation pending |
| Personal dictionary | Initial integration; system-word pinning and schema-v2 migration implemented, CI pending | Pin/unpin, clear, migration and ranking tests |
| Spell correction | Separate module, not integrated | Engine integration tests |
| Candidate popup | Basic native vertical Win32 popup implemented through TSF UIElement; hosted MSVC CI and live host-app verification pending | TSF UIElement contract tests, caret anchoring and monitor-work-area placement; keyboard/DPI/manual acceptance pending |
| Horizontal/vertical candidate window | Fixed vertical layout only; orientation setting and per-monitor DPI scaling are not implemented | Multi-monitor, DPI-change and orientation tests |
| Next-word prediction | Planned | Context evaluation corpus |
| Borno LM suggestions | Planned | Offline language-model evaluation |

## Windows desktop experience

| Feature | Status | Acceptance evidence |
|---|---|---|
| Windows COM/TSF service | Implementation present; validation ongoing | MSVC build and live typing |
| Password/PIN/private InputScope pass-through | Conservative TSF InputScope guard implemented; re-reads scope for every key and passes through unsupported/failed queries | InputScope unit tests plus live password/PIN controls; host app must disable TSF in password fields |
| Native Language Bar / Win+Space | Registration path present; live validation pending | Windows interactive test |
| Per-context composition | Implemented foundation | TSF contract and host-app tests |
| Candidate UIElement | COM/UIElement contracts and basic lifecycle implemented; host-driven rendering and interactive activation remain unverified | Count/string/selection/GUID/page-index contract tests; elevated TSF registration and live-app tests pending |
| Topbar | Planned | Interactive UI tests |
| System tray | Basic existing app support | Windows manual tests |
| Versioned General/Interface/Topbar/Input settings storage | Implemented, CI pending | V1 migration, V2 round-trip and validation tests |
| CapsLock, numpad and number-key preferences | Initial TSF runtime integration, CI pending | Keyboard-state translation and key-routing tests |
| Start with Windows preference persistence | Initial integration, CI pending | Registry rollback and settings tests |
| Remember last input layout | Implemented in engine startup, CI pending | Engine preference tests |
| Settings UI | Basic existing app support; full sections planned | Save/reload and UI tests |
| Theme system | Planned | Theme resource tests |
| On-Screen Keyboard | Planned | Layout-to-key action tests |
| Per-application profiles | Planned | Multi-process/window tests |
| DPI and multi-monitor support | Basic monitor work-area-aware popup placement exists; per-monitor DPI scaling and WM_DPICHANGED handling remain pending | Windows display/DPI matrix |
| Keyboard Layout Studio | Planned | Package validation/import/export tests |

## Packaging, services, and release

| Feature | Status | Acceptance evidence |
|---|---|---|
| Windows installer | Existing Inno Setup source | Fresh install/upgrade/uninstall |
| Selective layout installation | Planned | Installer task matrix |
| Custom layout packages | Planned | Validated package tests |
| About and license viewer | Planned | Resource and UI checks |
| Update checker | Planned | Signed manifest and rollback tests |
| SyncType Windows receiver | Planned | Pairing and local-network security tests |
| Android companion | Planned | Real-device testing |
| Voice-to-text provider | Planned | Consent and privacy tests |
| Offline typing path | Product requirement | Network-isolation tests |
| Production release acceptance | Not complete | All gates pass |

## Release gate rules

- Do not mark a row complete from a compile-only result.
- Windows-specific features require hosted MSVC build plus interactive verification.
- Language layouts require a maintained mapping corpus.
- External dictionaries and language models require provenance and license review.
- No third-party source code, parser, dictionary, model, UI asset or documentation
  may be copied into the MIT project. Behavioral mapping data must have traceable
  provenance and compatible licensing or written permission before distribution.
