# SUBORNO Production Readiness Blockers

**Status:** NOT PRODUCTION READY  
**Detailed phase tracker:** [PHASE-0-10-STATUS.md](PHASE-0-10-STATUS.md)

## Active release blockers

1. **Current-head CI:** Clang 23, GCC 13, and MSVC v143 Release/Debug must all pass
   on the exact candidate SHA. A previous commit's result is not evidence for a
   newer source state.
2. **Minimal ICU POC:** Must preserve ICU i18n functionality required by
   `icu::BreakIterator`, pass Unicode regression tests, and report reproducible
   binary/data size measurements before any production data-filter change.
3. **Phonetic compatibility:** An Avro clean-room parser update adds 299 documented
   conjunct input variants, strict consonant separation, contextual vowel behavior
   and regression vectors. Current-head CI and the behavioral-reference data's
   provenance/licensing review are still pending. Full Avro automatic vowel/reph,
   autocorrect and context behavior remain incomplete; Borno LM/context and Khipro
   full composition parity also remain incomplete.
4. **Physical layouts:** Probhat Base/Shift/AltGr/Shift+AltGr and CapsLock output
   layers are implemented with regression tests; OEM_102 and numpad decimal are
   recognized at the Windows virtual-key boundary. TSF probes actual session acceptance
   and passes unsupported combinations through. Current-head CI and interactive tests
   are pending; Probhat dead-key behavior remains incomplete. National physical mapping
   is blocked until BDS 1738:2018 licensing/permission is resolved; the current National
   module is a prototype, not a physical map.
5. **Unicode:** The Unicode 18 assigned Bengali-block inventory and general-category
   metadata have been expanded, excluding Assamese-specific U+09F0/U+09F1. Full
   corpus validation remains outstanding for malformed UTF-8, combining sequences,
   ZWJ/ZWNJ, cursor movement and grapheme-aware deletion.
6. **Dictionary and language quality:** Full dictionary corpus provenance,
   license clearance, ranking evaluation, spell-correction integration,
   next-word prediction and explicit learning controls remain outstanding.
7. **Windows TSF:** Hosted MSVC CI is necessary but not sufficient. Interactive
   activation, composition, candidate UI, keyboard navigation and compatibility
   must be tested in Notepad, Office and browsers.
8. **Desktop UX:** Full General/Interface/Input settings, Topbar, themes,
   on-screen keyboard, accessibility and per-monitor DPI behavior remain
   incomplete.
9. **Reliability and security:** Current-head sanitizer/fuzz/security/performance
   evidence, concurrency/lifetime review and supported Windows compatibility
   matrix remain outstanding.
10. **Installer/release:** Clean-machine install, upgrade, rollback and uninstall
    must pass. Production binaries require a valid signing certificate and
    verified signature/timestamp; a SHA-256 checksum is not a code signature.
11. **Other platforms:** Linux IBus/Fcitx and macOS InputMethodKit adapters are
    not production implementations; their current contracts do not constitute
    platform support.

## Environment-dependent blockers

- This repository session has no interactive Windows desktop host. Live TSF
  typing, language-bar activation, real host rendering, DPI and accessibility
  checks require a Windows session.
- Release signing requires the project's authorized certificate and release
  credentials. These must never be committed to the repository or exposed in logs.
- National layout implementation requires documented licensing clearance.

## Recovery and acceptance

- Fix CI failures with isolated changes and regression tests.
- Verify the exact latest PR head before recording any pass.
- Run the interactive Windows test matrix and clean-machine installer lifecycle.
- Complete license/provenance and security reviews.
- Keep the PR in draft until every gate in the phase tracker has evidence.
