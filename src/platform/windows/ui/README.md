# Windows UI

The native desktop application and notification-area integration live under
`src/platform/windows/app/`. The TSF DLL remains an in-process service and does
not own the desktop application's window or lifecycle.
