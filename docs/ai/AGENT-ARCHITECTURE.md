# Custom Agent Architecture

Workspace agents are stored in `.github/agents/`.

Essential agents:

- C++ architecture: ownership and portable API review.
- TSF/COM research: official Windows lifecycle review.
- Build: CMake, presets, vcpkg and MSVC/MSBuild review.
- Code review: repository standards and specification review.
- Quality: tests, sanitizers, fuzzing and evidence review.
- Documentation: source/document consistency.

Optional specialists:

- Security.
- Unicode/Bengali text processing.
- Dependency and license auditing.

Agents use read/search tools by default. They must not perform commit, push,
credential or destructive operations unless a separate approved agent contract
grants such work.
