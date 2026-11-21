# Technology Stack Decision

**Status:** Accepted for infrastructure setup

**Date:** 2026-11-17

- C++20 is the shared source language.
- Windows production builds use MSVC v143, Windows SDK, Win32, COM, and TSF.
  Windows artifacts are built on `windows-2022` GitHub-hosted runners.
- Ubuntu 24.04 is the primary developer environment, using Clang 23, clangd,
  CMake, Ninja, clang-format, and clang-tidy.
- CMake presets and vcpkg manifest mode govern configuration and dependency
  resolution. The registry baseline is pinned in `vcpkg.json`.
- ICU4C and SQLite are portable production dependencies. WIL is Windows-only.
  GoogleTest and Google Benchmark are development dependencies behind manifest
  features; benchmarks are disabled in ordinary builds.
- Linux, macOS, and Android input method adapters are not part of this decision.

This decision establishes tool and platform boundaries; it does not assert
that a production IME or TSF service exists.
