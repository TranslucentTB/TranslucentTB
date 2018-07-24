#pragma once
#include "arch.h"
#include <string>
#include <tchar.h>
#include <windef.h>
#include <winerror.h>

class Error {

private:
// Clang defines __FUNCSIG__ as a static const char[], but MSVC as a string literal.
// Which means on MSVC we can get a "free" wide character conversion with _T, but not
// on Clang, where we have to defer such conversion to runtime.
#ifdef __clang__
	using func_t = const char *const;
#define __TFUNCTION__ __FUNCSIG__
#else
	using func_t = const wchar_t *const;
#define __TFUNCTION__ _T(__FUNCSIG__)
#endif

public:
	enum class Level {
		Fatal,	// Show an error message to the user and immediatly exit
		Error,	// Show an error message to the user and log
		Log,	// Log to file and debug output
		Debug	// Log to debug output. For use in file log implementation.
	};

	static bool Handle(const HRESULT &error, const Level &level, const wchar_t *const message, const wchar_t *const file, const int &line, func_t function);
	static std::wstring ExceptionFromHRESULT(const HRESULT &result);
};

#define ErrorHandle(x, y, z) (Error::Handle((x), (y), (z), _T(__FILE__), __LINE__, __TFUNCTION__))
#define LastErrorHandle(x, y) (ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), (x), (y)))