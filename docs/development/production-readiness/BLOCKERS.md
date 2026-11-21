# SUBORNO Production Readiness Blockers

## Active blockers

1. The current environment has no Windows SDK/MSVC runtime; the Windows build must be verified by hosted CI.
2. Interactive Windows application compatibility and real typing remain untested.
3. Installer behavior on a clean Windows machine and release signing remain unverified.

## Recovery

- Run the repository's Windows MSVC GitHub Actions workflow and resolve failures.
- Perform interactive Windows activation, composition, and application-compatibility testing.
- Validate install/uninstall on a clean Windows machine and arrange release signing.
