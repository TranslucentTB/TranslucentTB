#pragma once
#include "arch.h"
#include <windef.h>

extern "C"
#ifdef EXPLORERTAP_EXPORTS
__declspec(dllexport)
#else
__declspec(dllimport)
#endif
HRESULT InjectExplorerTAP(DWORD pid, REFIID riid, LPVOID* ppv) noexcept;

using PFN_INJECT_EXPLORER_TAP = decltype(&InjectExplorerTAP);
