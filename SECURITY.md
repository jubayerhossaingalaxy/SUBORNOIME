# Security Policy

## Reporting a vulnerability

Please do not report exploitable vulnerabilities in public issues. Use
GitHub's private vulnerability reporting for this repository when available.
If private reporting is unavailable, contact the maintainers through a private
channel listed in the repository's GitHub security settings.

Do not include credentials, signing keys, personal data, or sensitive user
input in a report.

## Dependency and build security

- Production and development dependency versions are governed by
  `vcpkg.json` and its `builtin-baseline`.
- Review dependency notices and advisories before changing the baseline.
- The pinned vcpkg installation automatically generates per-package SPDX
  files under each installed port's `share/<port>/vcpkg.spdx.json`; its CLI
  does not expose a separate aggregate-SBOM subcommand. Syft reads each
  installed package SBOM, emits SPDX JSON, and Grype scans every resulting
  component file; refer to
  [third-party inventory](THIRD_PARTY_NOTICES.md) and the Ubuntu development
  guide for the verified command and current policy.
- Security findings are reported with severity and package context. The
  initial CI policy is informational: vulnerability findings do not
  automatically fail a build. Maintainers should prioritize confirmed Critical
  and High issues promptly, then assess Medium and Low advisories and any
  unresolved advisories. Review reachability, exploitability, affected package
  version, and available fixed versions; include severity counts and unresolved
  findings in the scan report.
- False-positive or not-affected decisions must include an explanation,
  evidence, and a review date in the relevant issue or pull request. Do not
  suppress findings silently.
- Upgrade to a fixed compatible version or document why an upgrade is blocked.

Windows signing certificates and other credentials must be stored only in
GitHub's protected secret storage, never in source control or workflow logs.
