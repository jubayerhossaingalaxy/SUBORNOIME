# Persistence

The portable persistence module is implemented in `atomic_file.cpp` and
provides temporary-file replacement for configuration data.

The settings module uses this abstraction to write versioned settings safely.
On Windows, replacement uses `MoveFileExW` so an existing settings file can be
replaced atomically. SQLite dictionary persistence is implemented separately
under `src/dictionary/` and is not part of this target.

Remaining acceptance includes interruption/failure injection, replacement
semantics on each supported filesystem, and real upgrade/rollback data-retention
tests.
