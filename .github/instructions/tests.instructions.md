---
applyTo: "tests/**,benchmarks/**"
---

- Use GoogleTest and existing CTest organization.
- Keep tests deterministic and independent.
- Use temporary database paths without fixed shared state.
- Separate unit, integration and interactive Windows test evidence.
- Document unrun Windows/IMM32 compatibility tests as untested.
