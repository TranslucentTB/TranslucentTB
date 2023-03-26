#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "taskbarappearanceservice.hpp"

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_DETACH:
		TaskbarAppearanceService::UninstallProxyStub();
		break;
	}

	return true;
}
