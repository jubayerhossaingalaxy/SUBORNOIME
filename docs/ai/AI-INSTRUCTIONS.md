# AI Instruction System

Repository-wide Copilot rules are in
[`.github/copilot-instructions.md`](../../.github/copilot-instructions.md).

Path instructions are in `.github/instructions/`:

- C++ source and headers.
- Windows/TSF/COM and MSVC configuration.
- tests and benchmarks.
- documentation.

Common rules stay in the central file. Path-specific rules do not repeat shared
constraints. An `AGENTS.md` file is not currently needed and will not be added
unless cross-harness support becomes an approved requirement.
