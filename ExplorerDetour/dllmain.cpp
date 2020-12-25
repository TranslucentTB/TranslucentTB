#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "explorerdetour.hpp"
#include "explorerhooks.hpp"
#include "timelinevisibilitymonitor.hpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// ignore errors, worse case scenario we get called for new threads
		DisableThreadLibraryCalls(hinstDLL);

		// Are we in Explorer?
		if (ExplorerHooks::IsInExplorer())
		{
			// Install the things
			ExplorerDetour::Install();
			TimelineVisibilityMonitor::Install();
		}
		break;

	case DLL_PROCESS_DETACH:
		TimelineVisibilityMonitor::Uninstall();
		ExplorerDetour::Uninstall();
		break;
	}

	return true;
}
