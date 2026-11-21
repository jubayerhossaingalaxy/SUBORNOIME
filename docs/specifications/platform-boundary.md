# Platform Boundary Specification

## Portable code

Portable C++20 targets must not include Windows headers or WIL. ICU and SQLite
are surfaced through dependency-specific targets.

- `suborno_layout_api` defines the host-neutral input-layout, input-session,
  candidate, and physical-key-event contracts.
- `suborno_dictionary` implements the SQLite-backed dictionary, normalized
  lookup, prefix lookup, and user-word persistence.
- `suborno_persistence` implements atomic file replacement for settings and
  other small application data; it has no SQLite dependency.
- `suborno_layout_sessions` provides shared stateless layout-session and
  candidate plumbing. Phonetic layouts retain their specialized parser and
  composition-session implementation.
- `suborno_unicode` owns UTF-8 validation, Unicode normalization, and
  grapheme segmentation through ICU.

## Windows-only code

The Windows shared-library target is added only when `WIN32` is true and must
use MSVC toolset v143. It may depend on WIL and Windows system APIs. The target
contains the COM class factory, in-process server registration, TSF language
profile registration, per-context composition sessions, keyboard event
handling, and TSF edit-session text insertion.

The Windows desktop executable owns the notification-area shell, layout
selection, startup preference, and Windows input settings integration. A
native TSF candidate UIElement and basic non-activating vertical popup are
implemented; host-driven rendering, keyboard interaction, accessibility,
and per-monitor DPI behavior still require acceptance testing. Topbar, theme
rendering, on-screen keyboard, and the full settings surface remain planned.

## Verification

Ubuntu CI compiles portable targets with Clang and GCC. Windows CI compiles the
Windows shared-library boundary and runs non-interactive tests on the hosted
Windows 2022 image. Real keyboard input and desktop integration require
interactive Windows verification outside ordinary hosted CI.
