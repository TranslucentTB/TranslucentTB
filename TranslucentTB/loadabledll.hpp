#pragma once
#include "arch.h"
#include <filesystem>
#include <libloaderapi.h>
#include <optional>
#include <string_view>
#include <wil/resource.h>

#include "util/null_terminated_string_view.hpp"
#include "../ProgramLog/error/win32.hpp"

class LoadableDll {
	wil::unique_hmodule m_hMod;

	static std::filesystem::path GetDllPath(const std::optional<std::filesystem::path>& storageFolder, std::wstring_view dll);
	static wil::unique_hmodule LoadDll(const std::filesystem::path &location);

public:
	LoadableDll(const std::optional<std::filesystem::path> &storagePath, std::wstring_view dll);

	template<typename T>
	static T GetProc(Util::null_terminated_string_view proc)
	{
		if (const auto ptr = GetProcAddress(m_hMod.get(), proc.c_str()))
		{
			return reinterpret_cast<T>(ptr);
		}
		else
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to get address of procedure");
		}
	}
};
