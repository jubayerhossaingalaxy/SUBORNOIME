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
tools rather than application runtime dependencies. The repository's third-party GitHub Actions are pinned to immutable
commit SHAs, with the upstream release tags recorded as comments in the workflow
files. Dependabot checks for updates.

## Avro phonetic behavioral reference (not a runtime dependency)

The Avro compatibility behavior specification at
`docs/specifications/avro-phonetic-behavior.md` records the public typing guide
consulted when authoring independent input/output regression vectors and the
corresponding token parser. The repository does not import the guide's parser
implementation, application source, dictionary, language model, binaries or
assets.

Reference: https://avro.aminul.dev/guide.html

The guide's licensing terms have not been established in this repository. This
attribution records provenance; it is not a license grant or permission to
redistribute the referenced mapping data. The Avro compatibility data must
receive a provenance/licensing review before the MIT project is distributed.

## Probhat keymap reference

The Probhat fixed physical-key output table is independently implemented from the MIT-licensed
princebillygk/probhat-layout-windows repository, layout.klc, upstream revision debc8310c60200143c6c0420433cca8506771e09.
The key-position, base, Shift, Ctrl+Alt/AltGr, Shift+Ctrl+Alt, and CapsLock output data are
referenced; Suborno does not copy the upstream installer or keyboard driver.

MIT License

Copyright (c) 2023 Prince Billy Graham Karmoker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Bundled Bengali system wordlist

- Project: [Sanjidh090/bangla-wordlist](https://github.com/Sanjidh090/bangla-wordlist)
- Upstream revision: `d3095ea05e5adcd55144e851d6aa1bb489af2e34`
- Upstream file: `words.txt` (Git blob `5a09ef7229911375ec70c5d3d7666cc9348752e2`)
- License: Unlicense public-domain dedication; the original license text is shipped as
  `assets/dictionaries/UNLICENSE-Sanjidh090.txt`.
- Dataset verification: 454,649 non-empty, unique entries; all observed code points are
  in the Bengali Unicode block; no U+09F0/U+09F1 entries were present.
- Packaging: the original UTF-8 list is split at line boundaries into four files without
  changing word contents or order. The first import normalizes the database copy to NFC
  and records the pinned corpus version in SQLite so concurrent or later app starts do
  not import the same corpus again.
- Limitations: this is a broad wordlist, not a curated normative dictionary. It includes
  inflected forms and proper names and provides no Latin transliterations, definitions,
  POS tags, or word-frequency evidence. Spell accuracy and ranking still require a
  linguistically reviewed evaluation corpus.

The dataset's Unlicense notice is reproduced unmodified in the shipped
`UNLICENSE-Sanjidh090.txt` file.

## License review

The listed licenses are permissive or public-domain-style. The ICU License
must be included when redistributing ICU, SQLite's source carries its public
domain dedication, and the MIT/BSD/Apache notices must be retained for their
respective dependencies. Preserve the license files/notices supplied by vcpkg
when packaging. This table is an inventory, not legal advice; re-check the
exact resolved port metadata before shipping binaries.
