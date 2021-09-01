#include "arch.h"
#include <spdlog/spdlog.h>
#include <windef.h>
#include <WinBase.h>
#include <WinUser.h>

#include "log.hpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		spdlog::shutdown();
		break;
	}

	return TRUE;
}
