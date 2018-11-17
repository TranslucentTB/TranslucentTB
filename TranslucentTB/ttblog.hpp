#pragma once
#include "arch.h"
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <windef.h>
#include <winrt/base.h>

class Log {
private:
	static std::mutex m_LogLock;
	static std::optional<winrt::file_handle> m_FileHandle;
	static std::wstring m_File;

	static std::pair<HRESULT, std::wstring> InitStream();

public:
	inline static bool init_done()
	{
		return m_FileHandle.has_value();
	}
	inline static const std::wstring &file()
	{
		return m_File;
	}
	static void __cdecl OutputMessage(std::wstring_view message);
	static void Flush();
};