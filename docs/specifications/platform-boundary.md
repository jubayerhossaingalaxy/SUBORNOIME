# Platform Boundary Specification

## Portable code

Portable C++20 targets must not include Windows headers or WIL. ICU and SQLite
are surfaced through dependency-specific targets. Intended module boundaries
are represented by `suborno_layout_api`, `suborno_dictionary`, and
`suborno_persistence`; these targets do not currently implement product logic.

## Windows-only code

The Windows shared-library target is added only when `WIN32` is true and must
use MSVC toolset v143. It may depend on WIL and Windows system APIs. The current
target is a DLL/TSF build boundary and does not implement COM registration or
a functioning TSF text service.

## Verification

Ubuntu CI compiles portable targets with Clang and GCC. Windows CI compiles the
Windows shared-library boundary and runs non-interactive tests on the hosted
Windows 2022 image. Real keyboard input and desktop integration require
interactive Windows verification outside ordinary hosted CI.
