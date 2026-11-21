# Automation Workflows

Existing workflows:

- Ubuntu Clang and GCC portability.
- Windows MSVC v143 build and CTest.
- SPDX/Syft and Grype dependency security.

New read-only ecosystem verification:

- `scripts/verify-ai-ecosystem.sh`
- `.github/workflows/ai-ecosystem.yml`

The verification script checks required instructions, agents, MCP JSON,
`skills-lock.json` and documentation files. It does not modify the working tree.

Future automation must not silently edit source, tests or configuration.
