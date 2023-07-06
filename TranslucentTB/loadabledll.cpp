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
	const auto [loc, hr] = win32::GetExeLocation();
	HresultVerify(hr, spdlog::level::critical, L"Failed to determine executable location!");

	std::filesystem::path dllPath = loc.parent_path() / dll;

	if (storageFolder)
	{
		// copy the file over to a place Explorer can read. It can't be injected from WindowsApps.
		auto tempDllPath = *storageFolder / L"TempState" / dll;

		std::error_code err;
		std::filesystem::copy_file(dllPath, tempDllPath, std::filesystem::copy_options::update_existing, err);
		if (err)
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
	else
	{
		return dllPath;
	}
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
