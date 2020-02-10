#include "arch.h"
#include <filesystem>
#include <sal.h>
#include <wil/resource.h>
#include <Unknwn.h>
#include <winrt/base.h>
#include <winrt/Windows.Storage.h>

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

std::filesystem::path GetConfigFileLocation(bool hasPackageIdentity)
{
	std::filesystem::path config_folder;
	if (hasPackageIdentity)
	{
		try
		{
			config_folder = std::wstring_view(winrt::Windows::Storage::ApplicationData::Current().RoamingFolder().Path());
		}
		HresultErrorCatch(spdlog::level::critical, L"Getting application folder paths failed!");
	}
	else
	{
		const auto [loc, hr] = win32::GetExeLocation();
		HresultVerify(hr, spdlog::level::critical, L"Failed to determine executable location!");
		config_folder = loc.parent_path();
	}

	config_folder /= CONFIG_FILE;
	return config_folder;
}

bool CheckAndRunWelcome(const std::filesystem::path &config_file, HINSTANCE hInst)
{
	if (!std::filesystem::is_regular_file(config_file))
	{
		Config { }.Save(config_file);
		if (!WelcomeDialog(config_file, hInst).Run())
		{
			std::filesystem::remove(config_file);
			return false;
		}
	}

	// Remove old version config once prompt is accepted.
	std::filesystem::remove_all(config_file.parent_path() / APP_NAME);

	return true;
}

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

	const bool hasPackageIdentity = UWP::HasPackageIdentity();
	auto config_file = GetConfigFileLocation(hasPackageIdentity);

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome(config_file, hInstance))
	{
		return EXIT_FAILURE;
	}

	// Initialize GUI
	MainAppWindow window(std::move(config_file), hasPackageIdentity, hInstance);

	// Run the main program loop. When this method exits, TranslucentTB itself is about to exit.
	return static_cast<int>(window.Run());
	// Not uninitializing WinRT apartment here because it will cause issues
	// with destruction of WinRT objects that have a static lifetime.
	// Apartment gets cleaned up by system anyways when the process dies.
}

#pragma endregion
