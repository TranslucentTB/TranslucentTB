#pragma once
#include "arch.h"
#include <roerrorapi.h>
#include <string>
#include <string_view>
#include <windef.h>
#include <winerror.h>
#include <winrt/base.h>

#include <util/strings.hpp>

class Error {
public:
	enum class Level {
		Fatal,	// Show an error message to the user and immediatly exit
		Error,	// Show an error message to the user and log
		Log,	// Log to file and debug output
		Debug	// Log to debug output. For use in file log implementation.
	};

	static bool HresultHandle(HRESULT error, Level level, std::wstring_view message, std::wstring_view file, unsigned int line);
	static void CppWinrtHandle(const winrt::hresult_error &err, Level level, std::wstring_view message, std::wstring_view file, unsigned int line);

	static std::wstring ExceptionFromHRESULT(HRESULT result);
	static std::wstring ExceptionFromIRestrictedErrorInfo(HRESULT hr, IRestrictedErrorInfo *info);

private:
	static const size_t PREFIX_LENGTH;

	static constexpr size_t GetPrefixLength(std::wstring_view filename)
	{
		const size_t pos = filename.find_last_of(LR"(/\)");
		return pos != std::wstring_view::npos ? pos + 1 : 0;
	}

	static void HandleCommon(Level level, std::wstring_view message, std::wstring_view error_message, std::wstring_view file, unsigned int line);
};

#define ErrorHandle(x, y, z) (Error::HresultHandle((x), (y), (z), UTIL_WIDEN(__FILE__), __LINE__))
#define LastErrorHandle(x, y) (ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), (x), (y)))
#define WinrtExceptionHandle(x, y, z) (Error::CppWinrtHandle((x), (y), (z), UTIL_WIDEN(__FILE__), __LINE__))
#define WinrtExceptionCatch(x, y) catch (const winrt::hresult_error &__err) { WinrtExceptionHandle(__err, (x), (y)); }