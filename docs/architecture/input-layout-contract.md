# Input Layout and Session Contracts

## Purpose

SUBORNO supports three independent input models:

- **Phonetic**: a sequence of Roman tokens is parsed into Bengali text.
- **Fixed**: each physical key or modifier layer maps to a deterministic output.
- **Compositional**: a sequence of keys is interpreted by a stateful layout-specific composition model.

These models must share the same host-facing session contract without sharing a phonetic parser.

## Module seams

- `InputLayout` describes the immutable behavior and kind of a layout.
- `InputSession` owns mutable, per-context composition state.
- `PhoneticLayout` uses `PhoneticInputSession` and the phonetic parser.
- `StatelessInputLayout` uses `LayoutInputSession` for layouts whose output can be rebuilt from the complete key sequence.
- A stateful compositional layout may provide its own `InputSession` implementation while satisfying the same interface.
- `LayoutRegistry` owns layout implementations; a session only observes its selected layout for the duration of its own context.

## Ownership and threading

- A registered layout is immutable after registration and is owned by the engine's registry.
- Each TSF context owns a distinct `InputSession`.
- Sessions are thread-confined to the TSF context/thread that created them.
- A session never owns its layout or dictionary; the engine outlives all sessions it creates.
- A session must not retain a TSF COM pointer or call Windows APIs.
- Candidate identifiers are stable within a layout's candidate model; UI code must not infer meaning from candidate positions.

## Session invariants

- Invalid UTF-8 and input-limit failures do not mutate the session.
- Backspace removes one Extended Grapheme Cluster from the input sequence.
- Candidate selection outside the current candidate range fails without mutation.
- Commit and reset clear the active composition state.
- Candidate generation is deterministic for the same input, layout, dictionary snapshot, and settings.
- Learning is applied only after a successful commit and only when enabled.

## Compatibility

The existing `PhoneticLayout` and `PhoneticInputSession` remain the compatibility path for the Suborno phonetic layout. The generic session contract is additive: it does not change the current phonetic parser rules or transliteration tables.

## Non-goals

The generic session contract does not itself define a physical keyboard matrix, national keyboard standard, candidate window, TSF edit session, desktop UI, or custom layout package format. Those are separate modules that consume this contract.
