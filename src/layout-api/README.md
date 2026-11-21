# Layout API

The public, platform-neutral layout contracts live under
`include/suborno/layout-api/`. They define keyboard events and modifiers,
layout metadata and registry interfaces, declarative mappings, candidates,
composition state, and input sessions.

The implementation files in this directory provide the layout registry and
declarative layout behavior. Concrete phonetic, compositional, and physical
layouts are separate modules under `src/layouts/`; native adapters translate
platform input into the shared contracts.

## Contract boundaries

- Layout definitions do not own platform-specific COM/TSF types.
- Input sessions own transient composition state.
- Physical layouts consume scan-code events; phonetic layouts consume UTF-8
  input.
- Candidate identity and text contracts are platform-neutral.

See `docs/architecture/input-layout-contract.md`,
`tests/unit/key_event_test.cpp`, and the layout/session unit tests.

The API and current implementations remain subject to ABI review, lifecycle,
concurrency, and cross-platform acceptance.
