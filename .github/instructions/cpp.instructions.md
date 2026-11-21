---
applyTo: "src/**/*.cpp,include/**/*.hpp"
---

- Use C++20, target-scoped CMake properties and existing formatting.
- Preserve API ownership and lifetime contracts.
- Prefer `std::unique_ptr`, value types and typed `Result<T>` styles.
- Add focused tests for behavior changes.
- Avoid Windows headers in platform-independent code.
