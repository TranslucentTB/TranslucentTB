#pragma once
#include "arch.h"
#include <roerrorapi.h>
#include <spdlog/common.h>
#include <string>
#include <string_view>
#include <windef.h>
#include <winerror.h>
#include <winrt/base.h>

#include "appinfo.hpp"
#include "util/strings.hpp"

namespace Error {
	std::wstring MessageFromHRESULT(HRESULT result);
	std::wstring MessageFromIRestrictedErrorInfo(IRestrictedErrorInfo *info);
	std::wstring MessageFromHresultError(const winrt::hresult_error &err);

	std::wstring FormatHRESULT(HRESULT result, std::wstring_view description);
	std::wstring FormatIRestrictedErrorInfo(HRESULT result, BSTR description);

	void HandleCommon(spdlog::level::level_enum level, std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function);

	inline bool MacroCommon(HRESULT hr, spdlog::level::level_enum level, std::wstring_view message, const char *file, int line, const char *function)
	{
		if (SUCCEEDED(hr))
		{
			return true;
		}
		else
		{
			HandleCommon(level, message, MessageFromHRESULT(hr), file, line, function);
			return false;
		}
	}
};

#define FATAL_ERROR_MESSAGE APP_NAME L" has encountered a fatal error and cannot continue executing."
#define ERROR_MESSAGE APP_NAME L" has encountered an error."

#define FATAL_ERROR_TITLE APP_NAME L" - Fatal error"
#define UTF8_ERROR_TITLE UTF8_APP_NAME " - Error"
#define ERROR_TITLE UTIL_WIDEN(UTF8_ERROR_TITLE)

#define __ERROR_LOCATION __FILE__, __LINE__, SPDLOG_FUNCTION
#define MessagePrint(__level, __message) (Error::HandleCommon((__level), (__message), { }, __ERROR_LOCATION))

#define HresultHandle(__hresult, __level, __message) (Error::MacroCommon((__hresult), (__level), (__message), __ERROR_LOCATION))
#define LastErrorHandle(__level, __message) (HresultHandle(HRESULT_FROM_WIN32(GetLastError()), (__level), (__message)))

#define HresultErrorHandle(__exception, __level, __message) (Error::HandleCommon((__level), (__message), Error::MessageFromHresultError((__exception)), __ERROR_LOCATION))
#define HresultErrorCatch(__level, __message) catch (const winrt::hresult_error &__exception) { HresultErrorHandle(__exception, (__level), (__message)); }
