#pragma once
#include "arch.h"
#include <string>
#include <utility>
#include <optional>
#include <windef.h>
#include <wrl/wrappers/corewrappers.h>

class Log {

private:
	using handle_t = Microsoft::WRL::Wrappers::FileHandle;
	static std::optional<handle_t> m_FileHandle;
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
	static void OutputMessage(const std::wstring &message);
	static void Flush();
};