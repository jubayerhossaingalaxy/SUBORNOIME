# Bengali Unicode support

SUBORNO's shared engine accepts UTF-8. ICU 79.1 supplies Unicode 18 data,
normalization, and Extended Grapheme Cluster boundaries. The native Windows
boundary may perform UTF-16 conversion; that adapter is outside the phonetic
engine milestone.

## UTF-8 and normalization

- UTF-8 is validated strictly before conversion. Overlong encodings, surrogates,
  out-of-range scalars, stray continuation bytes, and truncated sequences are
  invalid.
- `normalize_nfc` and the backward-compatible `normalize_nfkc` are explicit
  operations. Invalid UTF-8 is reported with `std::invalid_argument`; ICU
  initialization/normalization failures are reported with `std::runtime_error`.
- Phonetic output uses NFC. NFKC is not applied to composition output.
- Unicode canonical normalization does not preserve every precomposed Bengali
  nukta letter: for example, Unicode 18 NFC maps U+09DC to U+09A1 U+09BC.
  Consumers must not assume that NFC output retains U+09DC/U+09DD/U+09DF.
- `count_utf8_code_points` counts Unicode scalar values, not bytes.

## Bengali inventory and scope

The supported inventory records assigned Bengali-block characters in the
Unicode 18.0.0 U+0980–U+09FE range, including Bengali letters, signs, vowel
signs, digits, currency and historic symbols. Reserved code points are excluded.
The Assamese-specific additions U+09F0 BENGALI LETTER RA WITH MIDDLE DIAGONAL
and U+09F1 BENGALI LETTER RA WITH LOWER DIAGONAL are intentionally excluded
from both the inventory and `is_bengali_codepoint`.

The inventory's `category` field records the Unicode General_Category value.
An empty `romanization` means the character is recognized by the inventory but
does not yet have a default phonetic keystroke mapping. Inventory membership is
not, by itself, a promise that every character has a layout mapping or an
orthographic composition rule.

## Grapheme handling

`split_graphemes` uses ICU's root-locale character break iterator and follows
UAX #29 Extended Grapheme Cluster boundaries. Composition backspace applies
this to raw input. The output document's editing behavior remains the host's
responsibility.

## Bengali code-point helper

`is_bengali_codepoint` returns true only for assigned code points represented
in the supported Bangla inventory. It is not an orthographic validator,
transliterator, or keyboard-layout mapping lookup.

## References

- Unicode 18.0.0 Bengali code chart: https://www.unicode.org/Public/18.0.0/charts/PDF/U0980.pdf
- Unicode 18.0.0 Bengali names list: https://www.unicode.org/Public/18.0.0/charts/nameslist/0980/
- Unicode Normalization: https://www.unicode.org/reports/tr15/
- Unicode Text Segmentation: https://www.unicode.org/reports/tr29/
