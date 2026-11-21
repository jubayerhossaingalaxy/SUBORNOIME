# Settings

The portable settings model persists a versioned configuration containing the
active layout ID, candidate limit, and learning preference. Invalid values,
unknown keys, malformed files, and unsupported versions are reported through
`SettingsStatus`.

Saves write a temporary file and replace the destination; Windows uses
`MoveFileExW` to replace an existing settings file atomically. When an engine
loads an unknown layout ID, it falls back to its active default layout.
Composition sessions created by the engine apply the saved candidate limit and
do not record commits when learning is disabled.

There is no functional user-facing settings UI yet. The Windows settings
dialog remains incomplete.
