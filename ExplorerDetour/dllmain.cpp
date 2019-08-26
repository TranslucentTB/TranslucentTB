#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "explorerdetour.hpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hinstDLL);

		// Are we in Explorer?
		if (ExplorerDetour::IsInExplorer())
		{
			// Install the hook, fail otherwise.
			if (!ExplorerDetour::Install())
			{
				return false;
			}
		}
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		ExplorerDetour::Uninstall();
		break;
	}
	}

	return true;
}
