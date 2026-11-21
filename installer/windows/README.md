# Windows Installer

`suborno_installer.iss` builds `Suborno-Setup-x64.exe` (and
`Suborno-Setup-x86.exe` when compiled with `/dMyArchitecture=x86`) with
Inno Setup.

Each architecture's Program Files application directory contains exactly four
files after installation:

- `Suborno.exe`
- `THIRD_PARTY_NOTICES.md`
- Inno Setup's `unins000.exe`
- Inno Setup's `unins000.dat`

The third-party notice is displayed by the installer and is also copied beside
the application executable so users can inspect the licences after installation.

Shared machine-wide resource directories are created separately under
`%ProgramData%\\Suborno`: `assets\\fonts`, `assets\\themes`,
`dictionaries`, `layouts`, and
`components\\tsf\\{x86|x64}`. The TSF COM server DLL is installed in its
architecture-specific directory. The current installer source does not copy a
Bengali system wordlist, font/theme payload, or external layout package into
those resource directories; the portable dictionary database starts without a
bundled Bengali lexicon and learns words from user commits.

Per-user data lives under `%APPDATA%\\Suborno`: `settings.ini`,
`dictionaries\\user_candidates.db`,
`dictionaries\\suborno_learnt_bn`, and `layouts`. The startup preference
remains in HKCU.

The installer registers and enables the Bengali TSF language profile, and
creates Start Menu and Desktop shortcuts. The optional Quick Launch shortcut
is a legacy Windows feature. Re-running the installer removes a previous
version of the same architecture before installing, including residue from the
older in-directory deployment.

Uninstalling one architecture never touches the other architecture's shared
TSF resources or the user's `%APPDATA%\\Suborno` data. User data is never
deleted by the uninstaller. When the last architecture is uninstalled, only
empty shared directories under `%ProgramData%\\Suborno` are removed.

The Windows preset uses static x64/x86 vcpkg triplets and the static MSVC
runtime. ICU and SQLite are linked into the application modules rather than
copied as loose DLLs. Windows system DLLs are provided by Windows and are never
bundled.

The installer requires elevation because TSF COM registration is machine-wide.
