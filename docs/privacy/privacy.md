# Privacy and security boundaries for SUBORNO

SUBORNO does not transmit typed text or composition data over the network. Personal learning is disabled by default. If enabled through the settings file, committed words and typed aliases may be stored in the local user dictionary under the current user profile.

## Requirements

- No keystroke telemetry.
- No cloud sync of typed data.
- No third-party analytics SDK.
- No hidden background recording.
- No network dictionary sync in the typing path.

## User controls

- Personal learning is disabled by default; version 1 and version 2 settings migrate with learning disabled.
- When enabled, learned words and typed aliases are stored locally in the user dictionary; they are not uploaded.
- Settings and learned entries are stored locally and can be removed by deleting the profile database.
- Diagnostic logging must never include typed text content.
- Password/sensitive-field detection and automatic suppression of learning have not yet passed Windows host-app acceptance tests; keep personal learning disabled until that release gate is complete.
