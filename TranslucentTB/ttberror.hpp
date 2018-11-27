#pragma once
#include "arch.h"
#include <string>
#include <tchar.h>
#include <windef.h>
#include <winerror.h>

class Error {
public:
	enum class Level {
		Fatal,	// Show an error message to the user and immediatly exit
		Error,	// Show an error message to the user and log
		Log,	// Log to file and debug output
		Debug	// Log to debug output. For use in file log implementation.
	};

	static bool Handle(HRESULT error, Level level, const wchar_t *message, const wchar_t *file, int line, const wchar_t *function);
	static std::wstring ExceptionFromHRESULT(HRESULT result);
};

#define ErrorHandle(x, y, z) (Error::Handle((x), (y), (z), _T(__FILE__), __LINE__, _T(__FUNCSIG__)))
#define LastErrorHandle(x, y) (ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), (x), (y)))