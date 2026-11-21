# Ubuntu Environment Reference

The primary local environment is Ubuntu 24.04 LTS x86_64. Run
[`scripts/bootstrap-ubuntu.sh`](../../scripts/bootstrap-ubuntu.sh) to detect
the OS, install missing packages from Ubuntu repositories, install Grype from
Anchore's official installer, and bootstrap vcpkg at the pinned baseline.

Installed host versions and build evidence are recorded in the delivery report
and in command output, not hardcoded here. The project libraries are resolved
through vcpkg; preinstalled system ICU/SQLite headers are not used to satisfy
the manifest dependency graph.

Ubuntu does not provide the Windows SDK, MSVC COM headers, or TSF interfaces.
Use Windows CI for those compilation checks.
