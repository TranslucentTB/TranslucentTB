#include "loadabledll.hpp"
#include <format>
#include <wil/win32_helpers.h>

#include "resources/ids.h"
#include "localization.hpp"
#include "win32.hpp"
#include "windows/window.hpp"
#include "../ProgramLog/error/std.hpp"

std::filesystem::path LoadableDll::GetDllPath(const std::optional<std::filesystem::path> &storageFolder, std::wstring_view dll)
{
	std::error_code err;

	const auto [loc, hr] = win32::GetExeLocation();
	HresultVerify(hr, spdlog::level::critical, L"Failed to determine executable location!");

	std::filesystem::path dllPath = loc.parent_path() / dll;

	// copy the file over to a place Explorer can read. It can't be injected from WindowsApps.
	// when running portable, copy it to the temp folder anyways, to allow ejecting the device TTB is running from.
	std::filesystem::path tempFolder;
	if (storageFolder)
	{
		tempFolder = *storageFolder / L"TempState";
	}
	else
	{
		tempFolder = std::filesystem::temp_directory_path(err);
		if (!err)
		{
			tempFolder /= APP_NAME;
		}
		else
		{
			StdErrorCodeHandle(err, spdlog::level::critical, L"Failed to get temp folder path.");
		}
	}

	std::filesystem::create_directories(tempFolder, err);
	if (err) [[unlikely]]
	{
		StdErrorCodeHandle(err, spdlog::level::critical, L"Failed to create " APP_NAME " temp folder.");
	}

	auto tempDllPath = tempFolder / dll;
	std::filesystem::copy_file(dllPath, tempDllPath, std::filesystem::copy_options::update_existing, err);
	if (err) [[unlikely]]
	{
		if (err.category() == std::system_category() && err.value() == ERROR_SHARING_VIOLATION)
		{
			Localization::ShowLocalizedMessageBox(IDS_RESTART_REQUIRED, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND).join();
			ExitProcess(1);
		}
		else
		{
			StdErrorCodeHandle(err, spdlog::level::critical, std::format(L"Failed to copy {}", dll));
		}
	}

	return tempDllPath;
}

wil::unique_hmodule LoadableDll::LoadDll(const std::filesystem::path &location)
{
	wil::unique_hmodule hmod(LoadLibraryEx(location.c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
	if (!hmod)
	{
		LastErrorHandle(spdlog::level::critical, std::format(L"Failed to load {}", location.filename().native()));
	}

	return hmod;
}

LoadableDll::LoadableDll(const std::optional<std::filesystem::path> &storagePath, std::wstring_view dll) :
	m_hMod(LoadDll(GetDllPath(storagePath, dll)))
{
}
