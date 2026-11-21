# Unicode Module

The portable Unicode implementation is in `bengali.cpp` and is built as the
`suborno_unicode` target.

## Implemented behavior

- Strict UTF-8 validation, including malformed and truncated-sequence checks.
- Explicit NFC and NFKC normalization operations through ICU.
- Unicode scalar counting for UTF-8 input.
- ICU root-locale character BreakIterator segmentation for grapheme-aware
  backspace and composition boundaries.
- A Unicode 18 Bengali-block inventory with Unicode General_Category metadata.
  Assamese-specific U+09F0/U+09F1 and reserved code points are excluded from
  the supported Bangla inventory.

Grapheme segmentation requires ICU `uc` and `i18n`. The production target must
not drop `i18n` while it still calls `icu::BreakIterator`.

## Remaining acceptance

The Unicode module still needs the full Bengali corpus and host-editing tests
for ZWJ/ZWNJ behavior, cursor movement, selection replacement, and grapheme-aware
deletion in real TSF host applications.

See `docs/specifications/bengali-unicode-support.md` and
`tests/unit/unicode_test.cpp`.
