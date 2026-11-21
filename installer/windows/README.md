# Windows Installer

`suborno_installer.iss` builds `Suborno-Setup-x64.exe` (and `Suborno-Setup-x86.exe`
when compiled with `/dMyArchitecture=x86`) with Inno Setup. Each architecture's
Program Files application directory contains exactly `Suborno.exe` and Inno
Setup's `unins000.exe` / `unins000.dat`. Shared machine-wide resources are
installed separately under `%ProgramData%\Suborno\assets\fonts`,
`assets\themes`, `dictionaries`, and `layouts` with read/execute access for
users. Third-party notices are shown by the installer instead of being copied
into the application directory. The installer registers and enables the Bengali
language profile and creates Start Menu and Desktop shortcuts. The optional
Quick Launch shortcut is a legacy Windows feature. Re-running the installer
removes a previous version of the same architecture before installing, and
removes files left by the older in-directory deployment.

The three-layer data layout follows Borno's observable structure:

1. Application installation: `%ProgramFiles%\Suborno` (x64) or
   `%ProgramFiles(x86)%\Suborno` (x86), exactly three files.
2. Machine-wide shared resources: `%ProgramData%\Suborno` — installed fonts,
   themes, built-in dictionaries, default layouts, and the architecture-specific
   TSF COM server under `components\tsf\{x86|x64}`.
3. Per-user data: `%APPDATA%\Suborno` — `settings.ini`,
   `dictionaries\user_candidates.db`, `dictionaries\suborno_learnt_bn`, and
   `layouts`.

Uninstalling one architecture never touches the other architecture's shared
resources or the user's `%APPDATA%\Suborno` data. User data is never deleted by
the uninstaller; when the last architecture is uninstalled, empty shared
directories under `%ProgramData%\Suborno` are removed. The Windows preset uses
the static x64/x86 vcpkg triplet and static MSVC runtime. ICU and SQLite are
linked into the application modules rather than copied as loose DLLs. Windows
system DLLs are provided by Windows and are never bundled.

The installer requires elevation because the TSF COM registration is
machine-wide. The main application's settings, dictionaries, learned words, and
layouts live under `%APPDATA%\Suborno`; startup preference remains in HKCU.
