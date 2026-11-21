# Privacy and security boundaries for SUBORNO

SUBORNO does not transmit typed text or composition data over the network. Personal learning is disabled by default. Users can explicitly enable it from Desktop Settings; only compositions started after opt-in are eligible for learning. Turning learning off takes effect immediately. Committed words and typed aliases are stored locally in the user dictionary under the current user profile.

## Requirements

- No keystroke telemetry.
- No cloud sync of typed data.
- No third-party analytics SDK.
- No hidden background recording.
- No network dictionary sync in the typing path.

## User controls

- Personal learning is disabled by default; version 1 and version 2 settings migrate with learning disabled.
- Users explicitly opt in from Desktop Settings. Enabling learning during an existing composition does not retroactively learn that composition; it applies from the next composition. Disabling learning immediately stops recording, including for an active composition.
- When enabled, learned words and typed aliases are stored locally in the user dictionary; they are not uploaded.
- Settings and learned entries are stored locally. Learned words and personal dictionary entries can be removed from the Desktop Settings window with **Clear personal data**; the bundled Bengali system dictionary remains installed.
- Diagnostic logging must never include typed text content.
- Before capturing each key, the Windows TSF adapter re-reads the active selection's InputScope. Known password, PIN, and private scopes are passed through. If the host does not publish an InputScope, the selection cannot be read, or the scope query fails, Suborno passes the key through rather than capturing it. This conservative fallback may reduce compatibility with applications that do not expose TSF InputScope. Host applications must still disable text services in password fields; interactive Windows host-app acceptance tests remain outstanding.
- Personal learning remains disabled by default. Users can explicitly enable local learning from the desktop Settings window and clear learned/personal entries at any time; clearing personal data also turns learning off.
