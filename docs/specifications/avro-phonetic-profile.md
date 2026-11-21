# Avro Phonetic Profile

## Status

SUBORNO contains an independently authored Avro-style token profile and a
shared rule-based parser with accent-key, contextual consonant, and reph
primitives. This is an initial compatibility implementation, not complete
Avro Keyboard feature parity.

The public Avro Keyboard 3.1 user manual is used as a behavioral reference.
No Avro source code, mapping table, dictionary, test, or artwork is included.

## Input model

- Exact-case matching is used for case-sensitive tokens.
- Longer tokens are matched before their prefixes.
- Vowels use independent forms at a boundary and dependent forms after a
  consonant run.
- The inherent অ after a consonant is not emitted as a separate code point.
- Accent-key support can force an independent vowel, force a dependent sign,
  or separate adjacent consonants.
- The reph prefix token places র্ before the following consonant.
- Context-dependent consonant forms support ya-phala after an existing
  consonant run.
- Output is UTF-8 and normalized to NFC.

## Token reference

| Token | Output | Token | Output |
|---|---|---|---|
| a | আ / া | i | ই / ি |
| I, ee | ঈ / ী | u, oo | উ / ু |
| U | ঊ / ূ | rri | ঋ / ৃ |
| e | এ / ে | OI | ঐ / ৈ |
| o | অ / inherent অ | O | ও / ো |
| OU | ঔ / ৌ | k, kh | ক, খ |
| g, gh | গ, ঘ | Ng | ঙ |
| c | চ | ch, chh | ছ | j, jh | জ, ঝ |
| NG | ঞ | T, Th | ট, ঠ |
| D, Dh | ড, ঢ | N | ণ |
| t, th | ত, থ | d, dh | দ, ধ |
| n | ন | p | প |
| ph, f | ফ | b | ব |
| bh, v | ভ | m | ম |
| z | য | J, j | জ | r | র |
| l | ল | sh, S | শ |
| Sh | ষ | s | স |
| h | হ | R, Rh | ড়, ঢ় |
| y | য় at a boundary; য-ফলা after a consonant | Y | force য় independently |
| rr | reph prefix (র্) | ng | ং |
| t plus two accent keys | ৎ | Z | force য-ফলা at any position |
| ^ | ঁ | . | । |
| $ | ৳ | | |

## Accent-key model

The implementation uses the grave/backtick physical key as the profile's
accent key:

- A leading accent before a vowel forces the independent vowel form and breaks
  the preceding consonant run.
- An accent immediately after a vowel forces its dependent sign.
- An accent between consonants prevents the two consonants from being joined.
- A backslash remains the explicit literal-grapheme escape.

The mapping between the physical accent key and the character produced by
different Windows keyboard layouts is handled at the platform key-event layer.

## Explicit limitations

The following are not yet complete:

- Full Borno/Avro-style autocorrect dictionary management.
- Word/syllable-aware handling for every ambiguous consonant and inherent
  vowel case.
- The full historical Avro conjunct and alternative-candidate corpus.
- Full compatibility with every accent-key sequence from all Avro releases.
- User-facing Avro keymap/help viewer and layout-specific settings.
- A claim of byte-for-byte or behavior-for-behavior equivalence with Avro.

The feature-parity matrix remains authoritative for completion status.
