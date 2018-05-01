#pragma once
#include "arch.h"
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

	static bool Handle(const HRESULT &error, const Level &level, const wchar_t *const message, const wchar_t *const file, const int &line, const char *const function);
	static std::wstring ExceptionFromHRESULT(const HRESULT &result);
};

#define ErrorHandle(x, y, z) (Error::Handle((x), (y), (z), _T(__FILE__), __LINE__, __FUNCSIG__))