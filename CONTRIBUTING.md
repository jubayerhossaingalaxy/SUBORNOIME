# Contributing

SUBORNO is currently infrastructure-only. Keep changes within the approved
Windows-first, Ubuntu-developed C++20 scope unless a project decision says
otherwise.

## Repository conventions

- Put platform-independent APIs under `include/suborno/` and implementations
  in the corresponding `src/` area.
- Put Windows SDK, COM, TSF, and WIL usage exclusively under
  `src/platform/windows/`; do not make portable targets require those headers.
- Do not add product behavior, unsupported platform adapters, or third-party
  packages without an explicit design decision.
- Keep generated files, build output, `vcpkg_installed/`, and local paths out of
  version control.

## Branches and commits

Use short descriptive branch names such as `feature/core-boundaries` or
`fix/cmake-preset`. Use imperative commit subjects, preferably under 72
characters, and explain user-visible or architectural impact in the body.

## C++ standards

- C++20, RAII, explicit ownership, const-correctness, and no owning raw
  pointers.
- Keep compiler options target-scoped and platform-specific flags guarded by
  compiler checks.
- Format with the repository's Google-derived `.clang-format` policy.
- Do not silence warnings without explaining why.

## Local checks

```bash
scripts/configure-ubuntu.sh
scripts/build-ubuntu.sh
scripts/test-ubuntu.sh
scripts/check-format-ubuntu.sh
scripts/check-tidy-ubuntu.sh
```

Use `scripts/verify-ubuntu.sh` to run the full portable local verification
sequence. See [Ubuntu setup](docs/development/ubuntu.md) for setup and optional
benchmark commands.

## Pull requests

- Describe scope, design boundaries, and any dependency or license changes.
- Include focused tests for new behavior; do not claim a smoke target validates
  product functionality.
- Confirm format, static analysis, and Ubuntu tests pass.
- Windows-specific changes must be built and tested by the Windows CI job.
- Flag tests requiring an interactive desktop; hosted CI cannot validate
  physical keyboard interaction.
- Include no secrets, signing material, machine-specific paths, or generated
  dependency directories.
