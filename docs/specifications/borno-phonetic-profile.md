# Borno Phonetic Profile

## Status

The repository contains an independently authored, rule-based Borno-style
phonetic profile. This is the Regular Mode foundation, not a claim of complete
Borno feature parity. Context-aware language-model suggestions, advanced
autocorrection, and full literal-word escaping remain separate work.

The profile is implemented by:

- BornoPhoneticLayout
- RuleBasedPhoneticLayout
- LayoutInputSession

No Borno, BornoTSF, or Avro implementation source is included.

## Public behavior reference

The token behavior is based on the published Borno phonetic guide:
https://borno.codepotro.com/docs/guide/layout-borno.html

The reference is used to understand user-visible input behavior. The parser,
token storage, state transitions, and tests in SUBORNO are independently
authored.

## Token profile

| Input | Output | Input | Output |
|---|---|---|---|
| a | আ / া | o | অ / inherent অ |
| i | ই / ি | I, ee | ঈ / ী |
| u, oo | উ / ু | U | ঊ / ূ |
| rri | ঋ / ৃ | e | এ / ে |
| OI | ঐ / ৈ | O | ও / ো |
| OU | ঔ / ৌ | ng | ং |
| k, kh | ক, খ | g, gh | গ, ঘ |
| Ng | ঙ | c, ch | চ, ছ |
| j, jh | জ, ঝ | NG | ঞ |
| T, Th | ট, ঠ | D, Dh | ড, ঢ |
| N | ণ | t, th | ত, থ |
| d, dh | দ, ধ | n | ন |
| p, ph, f | প, ফ | b, bh, v | ব, ভ |
| m | ম | z | য |
| r, l | র, ল | sh, S | শ |
| Sh | ষ | s, h | স, হ |
| R, Rh | ড়, ঢ় | y, Y | য় |
| X, TH, t plus two backticks | ৎ | :: | ঃ |
| ^ | ঁ | . | । |

Uppercase tokens are deliberately case-sensitive. Longest exact-token matching
is used, so Th is distinct from t followed by h.

## Composition rules

1. A vowel at a word boundary emits its independent form.
2. A vowel after a consonant run emits its dependent sign. The token o emits no
   dependent sign because it represents the inherent vowel.
3. Adjacent consonants are joined by U+09CD VIRAMA.
4. An explicit ,, halant is reused by the following consonant rather than
   inserting a duplicate virama.
5. ,. and ,.. append virama plus ZWNJ or ZWJ.
6. ./ appends nukta only after an eligible consonant.
7. A backslash escapes the following Extended Grapheme Cluster.
8. ASCII digits are converted to Bengali digits.
9. Unknown valid Unicode text is preserved.
10. Output is normalized to NFC.

Invalid UTF-8 is rejected with std::invalid_argument.

## Deliberate limitations

The following are not implemented by this profile yet:

- Borno's optional LM Mode and contextual language-model ranking.
- Full Borno autocorrect behavior and user-managed autocorrect entries.
- The BornoTSF double-backslash literal-word behavior.
- Candidate ranking beyond the primary layout result and candidates supplied by
  the local dictionary.
- A claim of byte-for-byte or behavior-for-behavior equivalence with Borno.

These items must receive their own specifications and regression corpora before
they are marked complete.
