#include "arch.h"
#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <wil/resource.h>
#include "winrt.hpp"

#include "application.hpp"
#include "constants.hpp"
#include "mainappwindow.hpp"
#include "../ProgramLog/error/winrt.hpp"
#include "../ProgramLog/log.hpp"
#include "uwp/uwp.hpp"

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t *, int)
{
	const bool hasPackageIdentity = UWP::HasPackageIdentity();
	Log::Initialize(hasPackageIdentity);

	win32::HardenProcess();

	wil::unique_mutex mutex(MUTEX_GUID.c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// If there already is another instance running, tell it to exit
		MainAppWindow::CloseRemote();
	}

	try
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
	HresultErrorCatch(spdlog::level::critical, L"Initialization of Windows Runtime failed.");

	// Run the main program loop. When this method exits, TranslucentTB itself is about to exit.
	const auto ret = Application(hInstance, hasPackageIdentity).Run();

	// why are we brutally terminating you might ask?
	// Windows.UI.Xaml.dll likes to read null pointers if you exit the app too quickly after
	// closing a XAML window. While this is not a big deal for the user since we
	// are about to exit and saved everything, it pollutes telemetry and system crash data.
	// It's not easily doable to catch SEH exceptions in post-Main DLL unload, so instead just
	// brutally terminating will work.
	TerminateProcess(GetCurrentProcess(), ret);
	__fastfail(FAST_FAIL_FATAL_APP_EXIT);
}
