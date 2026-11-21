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
- The Windows TSF adapter checks the active selection's InputScope and passes through known password, PIN, and private scopes. This is a best-effort guard: host applications must disable text services in password fields, and the guard has not yet passed interactive Windows host-app acceptance tests.
- Personal learning remains disabled by default. Users can explicitly enable local learning from the desktop Settings window.
