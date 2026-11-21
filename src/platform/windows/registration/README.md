# Windows Registration

The Windows registration module implements COM class registration and TSF
language-profile registration in the Windows registry. It is used by the
installer and the TSF server lifecycle to expose the Suborno input service.

Registration code is architecture-aware and uses the matching 32-bit or 64-bit
registry view. It must be exercised on clean Windows installations, upgrades,
rollbacks and uninstall. Live language-bar activation, Win+Space switching and
host application behavior remain interactive acceptance requirements.
