#include "arch.h"
#include <synchapi.h>
#include <wil/resource.h>
#include "winrt.hpp"

#include "application.hpp"
#include "constants.hpp"
#include "mainappwindow.hpp"
#include "../ProgramLog/error/win32.hpp"

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t *, int)
{
	win32::HardenProcess();

	wil::unique_mutex mutex(MUTEX_GUID);
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
	// TODO: has app identity
	const auto exitCode = Application(hInstance, true).Run();

	winrt::uninit_apartment();
	return static_cast<int>(exitCode);
}

#pragma endregion
