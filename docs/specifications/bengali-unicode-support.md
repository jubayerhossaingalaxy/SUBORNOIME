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

## Grapheme handling

`split_graphemes` uses ICU's root-locale character break iterator and follows
UAX #29 Extended Grapheme Cluster boundaries. Composition backspace applies
this to raw input. The output document's editing behavior remains the host's
responsibility.

## Bengali range helper

`is_bengali_codepoint` currently tests the Bengali block range U+0980–U+09FF;
it is a range check, not a complete assigned-character or orthographic
validator. Rare historic, Vedic, and extended characters are not core phonetic
tokens.
