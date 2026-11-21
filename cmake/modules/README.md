# CMake Modules

Project-specific CMake helpers used by the root build.

- `SubornoBuildOptions.cmake` centralizes compiler warnings, hardening,
  sanitizer flags, and target-scoped build options.
- The root `CMakeLists.txt` owns the target graph and applies these options
  through `suborno_configure_target()`.

Add a module here only when it provides reusable build behavior needed by more
than one target or a clearly isolated platform-specific build concern.
