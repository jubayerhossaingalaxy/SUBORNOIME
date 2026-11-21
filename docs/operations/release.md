# Windows Release Requirements

## Required GitHub Actions secrets

Tag-based public releases are intentionally blocked until the Windows release
binaries are signed and trusted. Configure these repository Actions secrets:

- `WINDOWS_CODESIGN_CERTIFICATE_BASE64`: Base64-encoded PFX certificate with
  a code-signing EKU and a valid public trust chain.
- `WINDOWS_CODESIGN_CERTIFICATE_PASSWORD`: PFX password, if the PFX is
  password-protected.

The release workflow signs `Suborno.exe`, `suborno_windows_tsf.dll`, and
`Suborno-Setup-x64.exe` with SHA-256 and a trusted timestamp, then verifies
each signature before computing checksums or publishing the GitHub Release.
The PFX is written only to the ephemeral runner temp directory and removed
after signing. Never commit the certificate, password, or private key.

A manually dispatched workflow may produce an unsigned CI artifact for
installation testing. Such an artifact is not a public release and must not be
distributed as a production installer.

## Release acceptance gates

Before creating a release tag, maintainers must also confirm:

- Exact-head Windows MSVC Release and Debug builds and all CTest suites pass.
- Install, upgrade, reinstall, and uninstall lifecycle checks pass on a clean
  Windows runner.
- Interactive Windows 10/11 TSF typing is verified in supported host apps.
- Password and sensitive-field handling is validated; personal learning stays
  disabled unless that gate passes.
- Bengali dictionary corpus and phonetic/layout mappings have documented
  provenance and distribution rights.
- The selected layouts are labelled accurately; draft layouts are not advertised
  as production-supported.
- No unresolved Critical or High dependency advisories remain without an
  explicitly reviewed, evidence-backed disposition.

A green CI build by itself is not a production-readiness declaration.
