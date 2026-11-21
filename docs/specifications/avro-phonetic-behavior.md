# Avro phonetic behavior specification

## Scope

SUBORNO's Avro profile is an independently implemented compatibility target.
It uses a longest-match token parser, an explicit composition state machine and
a separately maintained set of observable input/output vectors. It does not
embed the Avro application, parser source, dictionary, language model or binary
assets.

## Implemented behavior in this revision

- Documented conjunct key sequences use longest-match tokens. The current
  regression corpus covers 299 input variants across the documented conjunct
  entries.
- A consonant pair is not automatically joined merely because two consonants
  were typed next to each other. Unmapped pairs remain separate; the backtick
  forces separation when an otherwise valid conjunct is intended to be written
  as independent consonants.
- Lowercase `a` uses its dependent sign after a consonant and the contextual
  ya-plus-aa form after an already written vowel/sign.
- Lowercase `o` uses the dependent form after a consonant and the documented
  independent `ও` form after an already written vowel.
- Word-initial lowercase `y` produces `ইয়`; uppercase `Y` remains the explicit
  `য়` key; conjunct entries provide the documented ya-phala forms.
- The backtick can force a full vowel or break a consonant sequence. Existing
  escaped-literal and special-sign rules remain covered by unit tests.

## Explicitly incomplete

- Full automatic vowel forming across all contexts.
- Old-style and automatic reph behavior for every consonant.
- The complete Avro autocorrect dictionary and personal replacements.
- Candidate dictionary parity and ranking behavior.
- All streaming/backspace/composition-session edge cases.
- Independent live Windows host behavior validation.

The Avro profile is not declared fully compatible or production ready by the
presence of this table or its tests. Acceptance requires current-head builds,
the complete compatibility suite, and manual host validation.

## Behavioral reference and provenance

The observable input/output examples used to author the compatibility vectors
were checked against the public Avro typing guide:

- https://avro.aminul.dev/guide.html

This repository does not import the guide's parser code or implementation.
The source guide's license status has not been established in this project;
therefore the compatibility data's provenance/license review remains a release
gate before distributing the MIT project. Do not treat the public URL alone as
a license grant.
