#pragma once
#include "arch.h"
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <windef.h>
#include <WinBase.h>

class Log {

private:
	static std::unique_ptr<std::wostream> m_LogStream;
	static std::tuple<HRESULT, std::wstring> InitStream();
	static std::wstring m_File;
	// The normal AutoLocalFree would call Error::Handle (which logs)
	// even tho we aren't done initializing the log stream.
	struct SilentLocalFreeDeleter {

		inline void operator() (void *data)
		{
			LocalFree(data);
		}

	};

public:
	static std::wstring file();
	static void OutputMessage(const std::wstring &message);
};