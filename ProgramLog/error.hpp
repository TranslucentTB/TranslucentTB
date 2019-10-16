#pragma once
#include "arch.h"
#include <cerrno>
#include <fmt/format.h>
#include "util/strings.hpp"
#include <rapidjson/error/en.h>
#include <roerrorapi.h>
#include <spdlog/common.h>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <windef.h>
#include <winerror.h>
#include <WinUser.h>
#include <winrt/base.h>

#include "api.h"
#include "appinfo.hpp"
#include "window.hpp"

#define FATAL_ERROR_MESSAGE APP_NAME L" has encountered a fatal error and cannot continue executing."
#define ERROR_MESSAGE APP_NAME L" has encountered an error."

#define FATAL_ERROR_TITLE APP_NAME L" - Fatal error"
#define UTF8_ERROR_TITLE UTF8_APP_NAME " - Error"
#define ERROR_TITLE UTIL_WIDEN(UTF8_ERROR_TITLE)

namespace Error {
	PROGRAMLOG_API std::wstring MessageFromHRESULT(HRESULT result);
	PROGRAMLOG_API std::wstring MessageFromIRestrictedErrorInfo(IRestrictedErrorInfo *info);
	PROGRAMLOG_API std::wstring MessageFromHresultError(const winrt::hresult_error &err);
	PROGRAMLOG_API std::wstring MessageFromErrno(errno_t err);
	PROGRAMLOG_API std::wstring MessageFromStdSystemError(const std::system_error &err);

	PROGRAMLOG_API std::wstring FormatHRESULT(HRESULT result, std::wstring_view description);
	PROGRAMLOG_API std::wstring FormatIRestrictedErrorInfo(HRESULT result, BSTR description);

	// Needs to be in DLL because spdlog log registry is per-module.
	PROGRAMLOG_API void Log(std::wstring_view msg, spdlog::level::level_enum level, const char *file, int line, const char *function);

	inline std::wstring GetLogMessage(std::wstring_view message, std::wstring_view error_message, std::wstring_view err_message_fmt = L"{} ({})", std::wstring_view message_fmt = L"{}")
	{
		if (!error_message.empty())
		{
			return fmt::format(err_message_fmt, message, error_message);
		}
		else
		{
			return fmt::format(message_fmt, message);
		}
	}

	template<spdlog::level::level_enum level>
	inline void Handle(std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function)
	{
		Log(GetLogMessage(message, error_message), level, file, line, function);
	}

	template<>
	inline void Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, const char* file, int line, const char* function)
	{
		Log(GetLogMessage(message, error_message), spdlog::level::err, file, line, function);

		std::thread([error = GetLogMessage(message, error_message, ERROR_MESSAGE L"\n\n{}\n\n{}", ERROR_MESSAGE L"\n\n{}")]
		{
			MessageBox(Window::NullWindow, error.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
		}).detach();
	}

	template<>
	[[noreturn]] inline void Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function)
	{
		Log(GetLogMessage(message, error_message), spdlog::level::critical, file, line, function);

		const std::wstring msg = GetLogMessage(message, error_message, FATAL_ERROR_MESSAGE L"\n\n{}\n\n{}", FATAL_ERROR_MESSAGE L"\n\n{}");
		MessageBox(Window::NullWindow, msg.c_str(), FATAL_ERROR_TITLE, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);

		// Calling abort() will generate a dialog box, but we already have our own.
		// Raising a fail-fast exception skips it but also allows WER to do its job.
		RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
	}

	template<spdlog::level::level_enum level>
	inline bool MacroCommon(HRESULT hr, std::wstring_view message, const char *file, int line, const char *function)
	{
		if (SUCCEEDED(hr))
		{
			return true;
		}
		else
		{
			Handle<level>(message, MessageFromHRESULT(hr), file, line, function);
			return false;
		}
	}
};

#define __ERROR_LOCATION __FILE__, __LINE__, SPDLOG_FUNCTION
#define MessagePrint(__level, __message) (Error::Handle<(__level)>((__message), { }, __ERROR_LOCATION))

#define HresultHandle(__hresult, __level, __message) (Error::MacroCommon<(__level)>((__hresult), (__message), __ERROR_LOCATION))
#define LastErrorHandle(__level, __message) (HresultHandle(HRESULT_FROM_WIN32(GetLastError()), (__level), (__message)))
#define ErrnoTHandle(__err, __level, __message) (Error::Handle<(__level)>((__message), Error::MessageFromErrno((__err)), __ERROR_LOCATION))
#define ParseErrorCodeHandle(__code, __level, __message) (Error::Handle<(__level)>((__message), rapidjson::GetParseError_En((__code)), __ERROR_LOCATION))

#define HresultErrorHandle(__exception, __level, __message) (Error::Handle<(__level)>((__message), Error::MessageFromHresultError((__exception)), __ERROR_LOCATION))
#define HresultErrorCatch(__level, __message) catch (const winrt::hresult_error &__exception) { HresultErrorHandle(__exception, (__level), (__message)); }

#define StdSystemErrorHandle(__exception, __level, __message) (Error::Handle<(__level)>((__message), Error::MessageFromStdSystemError((__exception)), __ERROR_LOCATION))
#define StdSystemErrorCatch(__level, __message) catch (const std::system_error &__exception) { StdSystemErrorHandle(__exception, (__level), (__message)); }
