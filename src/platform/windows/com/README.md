# Windows COM

The Windows TSF DLL is implemented as an in-process COM server. Its current
COM-related code is distributed across:

- `src/platform/windows/tsf/`: DLL entry point, class factory, text service,
  candidate UIElement and TSF edit sessions.
- `src/platform/windows/com/module_lifetime.hpp`: shared COM server module
  lifetime accounting.
- `src/platform/windows/registration/`: class, category and language-profile
  registration.

The COM class and TSF contracts have non-interactive Windows tests, but they
still require live registration/activation and typing verification in Notepad,
Office and supported browsers. The desktop application owns its own process and
window lifecycle; the TSF service must remain in-process and independent.
