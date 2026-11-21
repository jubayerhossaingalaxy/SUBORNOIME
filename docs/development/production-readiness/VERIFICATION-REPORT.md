# SUBORNO Verification Report

## Current Windows TSF changes

- Linux portable build with Clang 18: passed.
- CTest: 2/2 suites passed (unit and integration).
- Local formatting check: passed with clang-format 18; the hosted check uses clang-format 23.
- Local clang-tidy check: passed with clang-tidy 18.
- Local CTest: 2/2 suites passed after building the test targets.
- Local full build: passed with Clang 18.
- Hosted GitHub Actions run [44766128685](https://github.com/movielandbd1999-ops/SUBORNOIME/actions/runs/44766128685): passed on commit `7ab75c1`.
- Hosted Clang 23 formatting, clang-tidy, tests, and full build: passed.
- Hosted GCC 13 tests and full build: passed.
- Hosted Windows MSVC v143 Release/Debug contract tests and full builds: passed.
- Hosted Inno Setup installer compilation: passed.
- Interactive TSF typing and clean-machine installer behavior: not verified.

## Previously recorded evidence

- Ubuntu vcpkg configure/build/CTest passed.
- Unit test: passed.
- Integration test: passed.
- GCC 13 C++20 and existing engine probe passed.
- Portable Engine, layout registry and layout switching passed.
- Exact SQLite candidate integration passed.
- Formatting passed after clang-format-23.

## Historical repository state

- `main` branch.
- Existing AI ecosystem files are present.
- Existing source/CMake/CI files remain unchanged.

## Outstanding release evidence

- Interactive TSF/COM registration and typing.
- Clean-machine installer behavior and signing.
