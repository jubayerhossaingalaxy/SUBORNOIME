# Privacy and security boundaries for SUBORNO

SUBORNO does not collect or transmit typed text by default. The first release stores local dictionary and user settings in SQLite under the current user profile and avoids any network transmission of composition data.

## Requirements

- No keystroke telemetry.
- No cloud sync of typed data.
- No third-party analytics SDK.
- No hidden background recording.
- No network dictionary sync in the typing path.

## User controls

- User dictionary entries are local and optional.
- Settings are stored locally and can be removed by deleting the profile database.
- Diagnostic logging must never include typed text content.
