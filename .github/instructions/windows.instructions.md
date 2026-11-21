---
applyTo: "src/platform/windows/**,include/suborno/platform/windows/**,tests/windows/**,.github/workflows/windows-ci.yml,CMakeLists.txt,CMakePresets.json"
---

- Use native MSVC v143 and Windows SDK APIs only.
- Keep COM and TSF ownership inside the Windows adapter.
- Separate `ITfContext::RequestEditSession` outer `HRESULT` from session `HRESULT`.
- Verify deactivation, context loss and pending callback cleanup.
- Never claim interactive typing from a DLL smoke test.
