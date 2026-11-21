# ADR 0001: C++20 and MSVC v143 for the Windows production build

Status: Accepted

## Context

SUBORNO targets a native Windows x64 IME. The product requires strong interoperability with the Windows IME stack, COM, and TSF interfaces. The project must also support a lightweight Ubuntu-first development workflow.

## Decision

The project uses C++20 for all production and portable shared engine code. The Windows production compiler remains MSVC v143 and is required for the native x64 build. Clang and GCC are permitted only for Ubuntu portability validation and code quality checks.

## Consequences

- Windows production verification must run on a hosted MSVC runner or a matching developer machine.
- Ubuntu development remains lightweight and uses Clang with Ninja.
- The portable shared engine remains free of Windows-specific headers.
