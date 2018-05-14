#pragma once
#include "arch.h"
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <windef.h>
#include <WinBase.h>

#include "file.hpp"

class Log {

private:
	static std::unique_ptr<File> m_FileHandle;
	static std::pair<HRESULT, std::wstring> InitStream();
	static std::wstring m_File;

public:
	static const std::wstring &file();
	static void OutputMessage(const std::wstring &message);
	static void Flush();
};