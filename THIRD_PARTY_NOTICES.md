# Third-Party Dependency Inventory

The project consumes dependencies through the pinned vcpkg baseline
`0df93a05b9de080129e653ec4776d7e14a1b2e4f` (2026-11-16). The versions below
are the port versions at that baseline, not a claim that those packages have
been built on every supported platform. `vcpkg.json` constraints and features
define what each platform resolves.

| Package | Baseline version | Purpose | License | Platform / classification | Official source and maintenance |
|---|---:|---|---|---|---|
| ICU4C (`icu`) | 79.1 | Unicode support in the portable production core | ICU License | Ubuntu and Windows; production runtime dependency | [Unicode ICU](https://icu.unicode.org/), [releases](https://github.com/unicode-org/icu/releases); actively released. Upstream stable 79.1 was released 2026-10-22 and matches the vcpkg port. |
| SQLite (`sqlite3`) | 3.54.1 | Storage support in the portable production core | Public Domain / SQLite blessing | Ubuntu and Windows; production runtime dependency | [SQLite](https://sqlite.org/), [copyright](https://www.sqlite.org/copyright.html); maintained with regular releases. |
| Microsoft WIL (`wil`) | 1.0.260126.7 | Windows resource and COM helper utilities | MIT | Windows only; production-side Windows dependency | [Microsoft WIL](https://github.com/microsoft/wil); maintained by Microsoft. Not linked into Ubuntu targets. |
| GoogleTest (`gtest`) | 1.18.0 | Unit and infrastructure tests | BSD-3-Clause | Ubuntu and Windows; development-only | [GoogleTest](https://github.com/google/googletest); actively maintained by Google. |
| Google Benchmark (`benchmark`) | 1.10.0 | Optional performance benchmark executable | Apache-2.0 | Ubuntu and Windows; development-only and opt-in | [Google Benchmark](https://github.com/google/benchmark); actively maintained by Google. |

The vcpkg baseline pins the registry snapshot, while manifest minimum-version
constraints prevent silently selecting a port older than the reviewed
inventory. The resolved package graph is recorded by vcpkg under the ignored
build directory; verify it after configuring a platform.

vcpkg, CMake, Ninja, LLVM tools, GitHub Actions, and Grype are build/security
tools rather than application runtime dependencies. The repository's
third-party GitHub Actions are pinned to immutable commit SHAs; Dependabot
checks for updates.

## License review

The listed licenses are permissive or public-domain-style. The ICU License
must be included when redistributing ICU, SQLite's source carries its public
domain dedication, and the MIT/BSD/Apache notices must be retained for their
respective dependencies. Preserve the license files/notices supplied by vcpkg
when packaging. This table is an inventory, not legal advice; re-check the
exact resolved port metadata before shipping binaries.
