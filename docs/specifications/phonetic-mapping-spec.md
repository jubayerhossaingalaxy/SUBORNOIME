# SUBORNO phonetic mapping specification

The shared C++20 engine uses a clean-room, case-sensitive Roman-token registry.
The registry is immutable and parsed by longest exact-token match. It does not
lowercase input or contain word-specific exceptions. Dictionary correction is
not part of the phonetic parser.

## Vowels and parser state

| Token | Word start / after a vowel | After a consonant run |
|---|---|---|
| `a`, `aa` | আ | া |
| `ô`, `O` | অ | inherent অ; no sign |
| `i` | ই | ি |
| `ii`, `ee` | ঈ | ী |
| `u` | উ | ু |
| `uu`, `oo` | ঊ | ূ |
| `ri` | ঋ | ৃ |
| `e` | এ | ে |
| `oi`, `ai` | ঐ | ৈ |
| `o`, `w`, `W` | ও | ো |
| `ou`, `au` | ঔ | ৌ |

`a` and `ô` are never substituted for one another. A vowel after a consonant
run uses the dependent form and moves the parser to `AfterVowel`; a subsequent
vowel is independent. `ô` emits no dependent sign but still ends the
consonant run. Consequently, `kala` is কালা and `kôla` is কলা.

## Consonants and exact-case tokens

| Token(s) | Output |
|---|---|
| `k`, `q` | ক |
| `kh` | খ |
| `g`, `G` | গ |
| `gh`, `Gh` | ঘ |
| `c`, `C` | চ |
| `ch`, `Ch`, `chh` | ছ |
| `j`, `J`, `z`, `Z` | জ |
| `jh`, `Jh` | ঝ |
| `ny` | ঞ |
| `t` / `T` | ত / ট |
| `th` / `Th` | থ / ঠ |
| `d` / `D` | দ / ড |
| `dh` / `Dh` | ধ / ঢ |
| `n` / `N` | ন / ণ |
| `p`, `P` | প |
| `ph`, `Ph`, `f`, `F` | ফ |
| `b`, `B` | ব |
| `bh`, `Bh`, `v`, `V` | ভ |
| `m` | ম |
| `y` | য |
| `r` | র |
| `rh` | র + VIRAMA + হ |
| `R` | ড় |
| `Rh` | ঢ় |
| `Y` | য় |
| `l`, `L` | ল |
| `sh` | শ |
| `Sh` | ষ |
| `s`, `S` | স |
| `h` | হ |
| `M` | ং |
| `H` | ঃ |
| `ksh`, `kkh`, `x`, `X` | ক্ষ |
| `gy`, `jn`, `jny` | জ্ঞ |

Uppercase aliases not shown above are not implied. For example, `th` and `Th`
are distinct; `TH` is parsed as the exact tokens `T` then `H`, not as a
case-folded `th`.

The complete one-letter uppercase aliases are `A`→`a`, `B`→`b`, `C`→`c`,
`E`→`e`, `F`→`f`, `G`→`g`, `I`→`i`, `J`→`j`, `K`→`k`, `L`→`l`,
`P`→`p`, `Q`→`q`, `S`→`s`, `U`→`u`, `V`→`v`, `W`→`w`, `X`→`x`, and
`Z`→`z`. `O` means `ô`; `M`, `H`, `T`, `Th`, `D`, `Dh`, `N`, `R`, `Rh`,
`Sh`, and `Y` retain their exact mappings.

## Nasal and conjunct rules

- Standalone or word-final `ng` → ং. `ng` before a vowel → ঙ plus that vowel.
- `ng` before a consonant, and `n` before exact `k`, `kh`, `g`, or `gh`, produce
  primary ং followed by the consonant. No nasal alternate candidates are
  generated in the first release.
- Adjacent consonants form a run: U+09CD VIRAMA is inserted between them.
- `rk`, `kr`, `ky`, `kb`, and `km` therefore use the corresponding reph,
  rakar, ya-phala, ba-phala, and ma-phala codepoint sequences.
- `M` and `H` are sign tokens, not consonants; they never cause an automatic
  virama. In `Boundary` they emit their sign and move to `AfterVowel`; in
  `ConsonantRun` they emit after the consonant and close the run; in
  `AfterVowel` they emit and remain in `AfterVowel`.

## Alternatives and ranking

Longest-token primary matches are `ch`→ছ, `chh`→ছ, `sh`→শ, `ksh`/`kkh`→ক্ষ.
Alternatives consume the complete ambiguous span:

- `ch`: primary ছ; alternate চ + VIRAMA + হ; then alternate চ.
- `sh`: primary শ; alternate ষ; then alternate স + VIRAMA + হ.
- `chh`: primary ছ; alternate ছ + VIRAMA + হ.
- `ksh`: primary ক্ষ; alternate ক + VIRAMA + শ.
- `kkh`: primary ক্ষ; alternate ক + VIRAMA + খ.
- `ai`, `oi`, `au`, `ou`: compound vowel primary plus state-sensitive split
  vowel alternative. The split components go through normal parser state
  transitions.

When multiple ambiguous spans occur, the engine generates combinations, capped
at 16 candidates including the primary. Combinations are ordered by the sum of
their local alternate ranks, then by the first alternative's input position,
then lexicographically by per-span rank. Outputs are NFC-normalized and
deduplicated. Candidate score at zero-based rank `r` is `1/(r+1)`; it is ranking
metadata, not a probability.

## Controls and editing

| Input | Effect |
|---|---|
| `:boundary:` | End the current consonant run; emit nothing |
| `:danda:` / `:ddanda:` | Emit U+0964 / U+0965 |
| `:halant:` | Emit U+09CD after a valid consonant run |
| `:halant-zwnj:` / `:halant-zwj:` | Emit U+09CD plus the explicit joiner |
| `:zwnj:` / `:zwj:` | Add the joiner after an explicit terminal halant |
| `:nukta:` | Add U+09BC after an eligible consonant |
| `\\` followed by an EGC | Emit that EGC literally |

`:halant:` requires a consonant run; a following consonant reuses that halant
without duplicating U+09CD, while a following vowel is independent.
`:halant-zwnj:` and `:halant-zwj:` emit the virama plus joiner and end the run.
Standalone `:zwnj:` and `:zwj:` require an explicit terminal halant.
`:nukta:` requires a final eligible consonant without an existing nukta,
vowel sign, or virama. Invalid-context controls pass through literally. An
incomplete control or trailing escape remains pending during composition and is
literal on commit.
ASCII digits become Bengali digits; backslash-escaped digits remain ASCII.
Invalid UTF-8 and input-limit failures do not mutate a session. Backspace
removes one EGC from raw Roman input; editing committed output belongs to the
host.

## Examples

| Input | Primary output |
|---|---|
| `ami` | আমি |
| `bangla` | বাংলা |
| `bhalobasa` | ভালোবাসা |
| `bhalobasha` | ভালোবাশা |
| `bhasha` | ভাশা |
| `bhaSha` | ভাষা |
| `Dhaka` / `dhaka` | ঢাকা / ধাকা |
| `shob` / `sôb` | শোব / সব |
