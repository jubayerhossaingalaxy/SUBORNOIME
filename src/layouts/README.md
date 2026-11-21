# Input Layouts

Concrete layout implementations are kept independent behind the portable
contracts in `include/suborno/layout-api/`.

## Current modules

- `phonetic/`: shared rule-based parser/tokenizer plus separate Suborno,
  Avro-profile and Borno-regular-profile token tables and sessions.
- `khipro/`: initial compositional Khipro subset with forced vowel forms,
  slicer and separator handling; not full parity.
- `probhat/`: fixed physical scan-code table with Base, Shift, AltGr and
  Shift+AltGr layers plus KLC CapsLock flags. Dead-key parity is incomplete.
- `national/`: Roman-token prototype only; it is not the BDS 1738:2018
  physical map and must remain separate from licensed mapping research.

The layout modules are implementations, not claims of full parity. Each layout
requires maintained golden vectors and acceptance tests before release.
