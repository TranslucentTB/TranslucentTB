#include "arch.h"
#include <windef.h>
#include <WinBase.h>
#include <WinUser.h>

#include "log.hpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);
		Log::Initialize();
	}

	return TRUE;
}
