# Candidate Engine

The portable candidate engine is implemented in `candidate_engine.cpp` and
exposes a bounded `CandidateEngine::make_candidates` API.

## Candidate sources and ordering

1. Exact dictionary matches for the normalized primary layout output.
2. Dictionary matches for the raw input alias.
3. Prefix matches for the raw and normalized/transliterated input.
4. Layout-generated primary and alternative candidates.

Results are deduplicated by NFC-normalized candidate text and constrained by
the caller's candidate limit. Dictionary ordering uses the SQLite query's
frequency ranking; layout alternatives retain layout-provided order.

## Current boundaries

- Candidate generation is synchronous and part of the composition snapshot path.
- The separate spell-checker module is not integrated into this per-keystroke
  path; its bounded dictionary lookups need a latency budget before integration.
- Next-word prediction, contextual language modelling, candidate-choice
  analytics, and negative-feedback ranking are not implemented.
- Candidate source, normalized text, stable ID, score, and rationale are
  returned in the platform-neutral Candidate contract.

See `tests/unit/candidate_engine_test.cpp` and the production-readiness feature
parity matrix for current evidence and remaining acceptance work.
