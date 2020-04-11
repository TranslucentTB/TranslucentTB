#include "arch.h"
#include <filesystem>
#include <sal.h>
#include <wil/resource.h>
#include <Unknwn.h>
#include <winrt/base.h>

#include "constants.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "mainappwindow.hpp"
#include "uwp.hpp"
#include "taskdialogs/welcomedialog.hpp"
#include "win32.hpp"
#include "windows/windowclass.hpp"

bool OpenOrCreateMutex(wil::unique_mutex &mutex, const wchar_t *name)
{
	if (mutex.try_open(name))
	{
		return false;
	}
	else
	{
		mutex.create(name);
		return true;
	}
}

void InitializeWindowsRuntime() try
{
	winrt::init_apartment(winrt::apartment_type::single_threaded);
}
HresultErrorCatch(spdlog::level::critical, L"Initialization of Windows Runtime failed.");

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t *, int)
{
	win32::HardenProcess();

	wil::unique_mutex mutex;
	if (!OpenOrCreateMutex(mutex, MUTEX_GUID))
	{
		// If there already is another instance running, tell it to exit
		MainAppWindow::CloseRemote();
	}

	InitializeWindowsRuntime();

	// Initialize GUI
	MainAppWindow window(hInstance);

	// Run the main program loop. When this method exits, TranslucentTB itself is about to exit.
	return static_cast<int>(window.Run());
	// Not uninitializing WinRT apartment here because it will cause issues
	// with destruction of WinRT objects that have a static lifetime.
	// Apartment gets cleaned up by system anyways when the process dies.
}

#pragma endregion
