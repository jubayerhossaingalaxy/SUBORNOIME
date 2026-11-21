# SUBORNO Agent Skills Usage Guide

## Task Routing

| Engineering task | Skill |
|---|---|
| C++ architecture, ownership, PIMPL, RAII or typed errors | `modern-cpp` |
| CMake target graph, presets, MSVC/MSBuild or dependencies | `cmake` |
| GoogleTest/CTest test design, fuzzing, sanitizers or flakes | `cpp-testing` |
| vcpkg manifest, version baseline or triplet review | `vcpkg` |
| Module seams, interface depth or adapter placement | `codebase-design` |
| Review a branch/diff against repository standards and specification | `code-review` |
| Architecture and dependency Mermaid diagrams | `design-doc-mermaid` |

## Practical Verification Completed

These findings were produced by applying the installed Skills' workflows to the
existing repository. They are research findings, not source changes.

### Modern C++ and ownership

The `modern-cpp` workflow identified [dictionary.hpp](include/suborno/dictionary/dictionary.hpp)
as a PIMPL ownership point. The public type owns `Impl*` but does not yet
explicitly delete copy operations or document move semantics. This is a
potential copy-safety risk, not a demonstrated runtime double-delete. The
recommended future contract is RAII PIMPL ownership or explicit non-copy/move
semantics, plus typed SQLite failures.

### CMake and MSVC configuration

The `cmake` workflow confirmed the target graph is modular and target-scoped.
The Windows production build already rejects unsupported compilers, includes
`MSVC_TOOLSET_VERSION 143`, and builds a native x64 DLL. It should not be
expanded with a fallback compiler.

The CMake test graph remains a buildable scaffold: no real TSF or COM target
is exported. `suborno_windows_tsf` is still a smoke boundary.

### C++ testing

The `cpp-testing` workflow confirmed that existing GoogleTest coverage is
substantial for parser, tokenizer, Unicode and session behavior but does not
cover COM activation, edit locks, composition lifecycle, dictionary migration
or a Windows application matrix. Future tests should remain isolated and use
deterministic fixtures.

### vcpkg

The `vcpkg` workflow confirmed manifest mode is already in use with a pinned
`builtin-baseline`, direct ICU/SQLite/WIL dependencies and a testing feature.
Windows package selection must remain x64/MSVC-compatible.

### Codebase design

The `codebase-design` workflow found that `CandidateEngine` is currently a
pass-through rather than a deep candidate-ranking module. `Dictionary` and
layout alternatives are separate, as they should be, but no Engine-owned seam
currently combines them.

`PhoneticInputSession` has a concrete layout dependency and creates
`SubornoPhoneticLayout` internally. Future composition sessions need layout
injection or a layout handle supplied by a registry/Engine; the interface
should hide layout construction details.

### Code review

The `code-review` Skill was inspected and is suitable for future diffs only
after a fixed Git reference and originating specification are available.
It was not used to claim findings on this repository because no implementation
diff exists.

### Mermaid documentation

The Mermaid Skill was used conceptually to validate the revised roadmap's
module and implementation dependency diagrams. No Mermaid helper script was
executed and no diagram image was generated.

## Project Constraints for Every Skill

- Windows production code: MSVC v143, Visual Studio 17 2022, CMake, MSBuild,
  Windows SDK and x64 only.
- Ubuntu is a portability and CI environment; it is not a Windows production
  compiler fallback.
- C++20 is the shared-engine standard.
- No custom desktop UI, tray app, dashboard or candidate window.
- No source implementation is to be started solely because a Skill was installed.
- Official Microsoft TSF/COM/Windows documentation and repository evidence
  override generic examples in a Skill.
