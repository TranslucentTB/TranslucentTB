#pragma once
#include "arch.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <windef.h>
#include <wil/resource.h>

class Log {
private:
	static std::optional<wil::unique_hfile> m_FileHandle;
	static std::filesystem::path m_File;

	static std::filesystem::path GetPath();
	static std::pair<HRESULT, std::wstring> InitStream();

public:
	inline static bool init_done() noexcept
	{
		return m_FileHandle.has_value();
	}
	inline static const std::filesystem::path &file() noexcept
	{
		return m_File;
	}
	static void OutputMessage(std::wstring_view message);
	static void Flush();
};