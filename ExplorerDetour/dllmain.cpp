#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "explorerdetour.hpp"
#include "util/abort.hpp"

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
			// Install the hook
			return ExplorerDetour::Install();
		}
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		if (!ExplorerDetour::Uninstall())
		{
			// state may have been compromised, kill the process
			Util::QuickAbort();
		}
		break;
	}
	}

	return true;
}
