#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "visualtreewatcher.hpp"

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_DETACH:
		VisualTreeWatcher::UninstallProxyStub();
		break;
	}

	return true;
}
