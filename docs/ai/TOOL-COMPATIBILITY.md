# Tool Compatibility

## Existing local environment

- CMake presets and CMake Tools.
- clangd and clang-format.
- VS Code C++ and CMake extensions.
- Ubuntu Clang/GCC portability CI.
- Windows VS 2022/MSVC v143 CI.

## Resource policy

The development PC has approximately 4 GB RAM:

- no Docker or container stack is required;
- no local LLM or GPU service is required;
- Windows builds remain single-job;
- background services must remain optional and lightweight.

## Platform policy

Ubuntu tooling does not determine the Windows production compiler.
Windows release builds use only MSVC v143, MSBuild, CMake and Windows SDK.
