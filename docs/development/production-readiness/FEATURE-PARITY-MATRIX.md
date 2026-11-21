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
| Borno Phonetic Regular Mode | Initial implementation | borno_phonetic_layout_test.cpp; CI pending |
| Borno Phonetic LM Mode | Planned | Context corpus and ranking evaluation |
| Avro Phonetic | Planned | Official-behavior regression corpus |
| Khipro compositional layout | Planned | Stateful composition transition corpus |
| National physical key layout | Not yet verified as fixed layout | Full physical-key matrix |
| Probhat physical key layout | Not yet verified as fixed layout | Full physical-key matrix |
| Phonetic literal escaping | Partial | Single-EGC escape tests |
| Automatic vowel forming | Planned | Avro compatibility vectors |
| Old-style/automatic reph | Planned | Reph composition corpus |
| ANSI/Bijoy encoding | Planned | Converter and legacy-app tests |

## Candidate and language intelligence

| Feature | Status | Acceptance evidence |
|---|---|---|
| Layout primary candidate | Implemented | Layout unit tests |
| Dictionary candidate source | Implemented | Candidate and session tests |
| Candidate-choice learning | Partial | Requires event/ranking model |
| Personal dictionary | Existing foundation | Persistence tests |
| Spell correction | Separate module, not integrated | Engine integration tests |
| Candidate popup | Planned | TSF UIElement and interactive tests |
| Horizontal/vertical candidate window | Planned | DPI/positioning tests |
| Next-word prediction | Planned | Context evaluation corpus |
| Borno LM suggestions | Planned | Offline language-model evaluation |

## Windows desktop experience

| Feature | Status | Acceptance evidence |
|---|---|---|
| Windows COM/TSF service | Implementation present; validation ongoing | MSVC build and live typing |
| Native Language Bar / Win+Space | Registration path present; live validation pending | Windows interactive test |
| Per-context composition | Implemented foundation | TSF contract and host-app tests |
| Candidate UIElement | Planned | UIElement lifecycle tests |
| Topbar | Planned | Interactive UI tests |
| System tray | Basic existing app support | Windows manual tests |
| Settings UI | Basic existing app support | Save/reload and UI tests |
| Theme system | Planned | Theme resource tests |
| On-Screen Keyboard | Planned | Layout-to-key action tests |
| Per-application profiles | Planned | Multi-process/window tests |
| DPI and multi-monitor support | Planned | Windows display matrix |
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
- No third-party source, mapping table, dictionary, test, UI asset, or documentation
  may be copied into the MIT project.
