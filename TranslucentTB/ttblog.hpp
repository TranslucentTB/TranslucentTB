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
	static const std::wstring &file();
	static void OutputMessage(const std::wstring &message);
	static void Flush();
};