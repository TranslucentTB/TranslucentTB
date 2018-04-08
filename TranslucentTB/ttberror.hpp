#pragma once
#include "arch.h"
#include <codecvt>
#include <locale>
#include <string>
#include <tchar.h>
#include <windef.h>

class Error {

public:
	enum class Level {
		Fatal,	// Show an error message to the user and immediatly exit
		Error,	// Show an error message to the user and log to debug output
		Log		// Log to debug output
	};

	static bool Handle(const HRESULT &error, const Level &level, const std::wstring &message, const std::wstring &file, int line, const std::wstring &function);
};

#define ErrorHandle(x, y, z) (Error::Handle((x), (y), (z), _T(__FILE__), __LINE__, std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(__FUNCSIG__)))