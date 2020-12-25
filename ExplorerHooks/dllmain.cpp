#include "arch.h"
#include <libloaderapi.h>
#include <windef.h>

#include "explorerhooks.hpp"
#include "swcadetour.hpp"
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
			SWCADetour::Install();
			TimelineVisibilityMonitor::Install();
		}
		break;

	case DLL_PROCESS_DETACH:
		TimelineVisibilityMonitor::Uninstall();
		SWCADetour::Uninstall();
		break;
	}

	return true;
}
