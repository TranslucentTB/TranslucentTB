#pragma once
#include "arch.h"
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <windef.h>
#include <WinBase.h>

class Log {

private:
	static std::unique_ptr<std::wostream> m_LogStream;
	static std::pair<HRESULT, std::wstring> InitStream();
	static std::wstring m_File;

public:
	static std::wstring file();
	static void OutputMessage(const std::wstring &message);
};