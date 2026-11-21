# SUBORNO AI Agent Skills Inventory

## Scope

This project-level inventory covers Skills installed under `.agents/skills/` for
GitHub Copilot. The CLI also records pinned sources in `skills-lock.json`.
No application source, CMake, test, branch, or build configuration was changed.

## Installed Skills

| Skill | Category | Source | Commit | Installation | GitHub Copilot use | Security status |
|---|---|---|---|---|---|---|
| `modern-cpp` | Modern C++20/architecture | [trailofbits/skills](https://github.com/trailofbits/skills) | `149d7b21f4067765b82e67029730b42d228ef4b4` | [`.agents/skills/modern-cpp/`](.agents/skills/modern-cpp/) | Ownership, PIMPL, RAII, typed errors, smart pointers, thread-safety and C++20 idioms | Skills.sh risk assessment: Safe; Socket 0 alerts; Snyk Low Risk |
| `cmake` | CMake/build engineering | [mohitmishra786/low-level-dev-skills](https://github.com/mohitmishra786/low-level-dev-skills) | `d0d0a13d4e3caec7afb2d647a1fffdb7cb1ee058` | [`.agents/skills/cmake/`](.agents/skills/cmake/) | Target-first CMake, presets, dependency boundaries and sanitizer integration | Skills.sh risk assessment: Safe; Socket 0 alerts; Snyk Low Risk |
| `cpp-testing` | C++ testing | [affaan-m/ecc](https://github.com/affaan-m/ecc) | `4adeee6732a12d1540f49a4d50a38ce8c97f9a53` | [`.agents/skills/cpp-testing/`](.agents/skills/cpp-testing/) | GoogleTest/CTest, isolation, flake diagnosis, sanitizers and fuzzing boundaries | Skills.sh risk assessment: Safe; Socket 0 alerts; Snyk Low Risk |
| `vcpkg` | Dependency management | [github/awesome-copilot](https://github.com/github/awesome-copilot) | `b0b0948ece4c25a83fe1243c400f57ae28afefaf` | [`.agents/skills/vcpkg/`](.agents/skills/vcpkg/) | Manifest mode, pinned baselines, triplets, features and CI cache review | Skills.sh risk assessment: Safe; Socket 0 alerts; Snyk Med Risk |
| `codebase-design` | Module and interface design | [mattpocock/skills](https://github.com/mattpocock/skills) | `abeeaf95660ba9e8b7c753eb38f441ca707a8823` | [`.agents/skills/codebase-design/`](.agents/skills/codebase-design/) | Module seams, interface depth, adapter boundaries and test surfaces | Skills.sh risk assessment: Safe; Socket 0 alerts; Snyk Low Risk |
| `code-review` | Two-axis code review | [mattpocock/skills](https://github.com/mattpocock/skills) | `abeeaf95660ba9e8b7c753eb38f441ca707a8823` | [`.agents/skills/code-review/`](.agents/skills/code-review/) | Standards/spec review of future diffs | Skills.sh risk assessment: Safe; Socket 0 alerts; Snyk Med Risk |
| `design-doc-mermaid` | Architecture documentation | [SpillwaveSolutions/design-doc-mermaid](https://github.com/SpillwaveSolutions/design-doc-mermaid) | `c36d83503d2ba9c1e51638dc2d8a34758a377dea` | [`.agents/skills/design-doc-mermaid/`](.agents/skills/design-doc-mermaid/) | Mermaid architecture, dependency and sequence diagrams | Skills.sh risk assessment: Safe; Socket 0 alerts; Snyk Med Risk |

## Maintenance

Update project Skills:

```bash
npx skills update
```

Update only project Skills:

```bash
npx skills update -p
```

Restore the pinned lockfile state:

```bash
npx skills experimental_install
```

List project Skills:

```bash
npx skills list --json
```

Remove a Skill:

```bash
npx skills remove <skill-name>
```

Remove a project Skill from GitHub Copilot:

```bash
npx skills remove <skill-name> --agent github-copilot
```

## Known Limitations

- No dedicated, source-inspected skills.sh TSF/IME, COM, MSVC, ICU, or native Windows text-service Specialist was found.
- `bengali-nlp` is cross-language NLP guidance rather than a verified C++/ICU implementation Skill, so it was not installed.
- The CMake Skill includes generic GCC/Clang and cross-compilation examples. SUBORNO's Windows production compiler remains MSVC v143; Ubuntu remains a portability environment.
- The Modern C++ Skill contains C++26 suggestions. SUBORNO's implementation standard remains C++20 unless a separate approved roadmap change is made.
- Skills.sh leaderboard popularity is telemetry, not evidence of Skill correctness or security.
- Skills.sh's public ranking page and CLI state that no security guarantee is made by the ecosystem.

## Security Notes

- Source repositories and selected `SKILL.md` files were reviewed before installation.
- The selected text-only Skills do not request credentials.
- The Mermaid package includes Python helper scripts. They use local files and invoke an optional `mmdc` executable only when explicitly requested; they do not contain credential access or destructive shell commands in the inspected scripts.
- A Skill may run with full agent permissions after installation. Only project-local content and pinned commit references are trusted for this repository.
