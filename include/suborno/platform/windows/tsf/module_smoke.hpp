#pragma once

#if defined(SUBORNO_WINDOWS_TSF_EXPORTS)
#define SUBORNO_WINDOWS_TSF_API __declspec(dllexport)
#else
#define SUBORNO_WINDOWS_TSF_API __declspec(dllimport)
#endif

extern "C" SUBORNO_WINDOWS_TSF_API unsigned int suborno_windows_module_smoke() noexcept;
