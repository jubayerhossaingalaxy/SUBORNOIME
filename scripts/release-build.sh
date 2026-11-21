cmake --build build\windows-msvc --config Release --parallel 1
cmake --build build\windows-msvc --config Release --target suborno_app --parallel 1
ISCC installer\windows\suborno_installer.iss
