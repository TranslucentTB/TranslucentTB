#pragma once
#include "arch.h"
#include <windef.h>

extern "C"
#ifdef EXPLORERHOOKS_EXPORTS
__declspec(dllexport)
#else
__declspec(dllimport)
#endif
HHOOK InjectExplorerHook(HWND window) noexcept;

using PFN_INJECT_EXPLORER_HOOK = decltype(&InjectExplorerHook);
