#pragma once
#include "arch.h"
#include <fmt/format.h>
#include <roerrorapi.h>
#include <spdlog/common.h>
#include <string>
#include <string_view>
#include <thread>
#include <windef.h>
#include <winerror.h>
#include <WinUser.h>
#include <winrt/base.h>

#include "api.h"
#include "appinfo.hpp"
#include "util/strings.hpp"
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

	PROGRAMLOG_API std::wstring FormatHRESULT(HRESULT result, std::wstring_view description);
	PROGRAMLOG_API std::wstring FormatIRestrictedErrorInfo(HRESULT result, BSTR description);

	// Needs to be in DLL because spdlog log registry is per-module.
	PROGRAMLOG_API void Log(std::wstring_view msg, spdlog::level::level_enum level, const char *file, int line, const char *function);

	template<spdlog::level::level_enum level>
	inline void Handle(std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function)
	{
		std::wstring msg;
		if (!error_message.empty())
		{
			msg = fmt::format(fmt(L"{} ({})"), message, error_message);
		}
		else
		{
			msg = message;
		}

		Log(msg, level, file, line, function);
	}

	template<>
	inline void Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, const char* file, int line, const char* function)
	{
		std::wstring msg;
		if (!error_message.empty())
		{
			msg = fmt::format(fmt(ERROR_MESSAGE L"\n\n{}\n\n{}"), message, error_message);
		}
		else
		{
			msg = fmt::format(fmt(ERROR_MESSAGE L"\n\n{}"), message);
		}

		Log(msg, spdlog::level::err, file, line, function);

		std::thread([error = std::move(msg)]
		{
			MessageBox(Window::NullWindow, error.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
		}).detach();
	}

	template<>
	[[noreturn]] inline void Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function)
	{
		std::wstring msg;
		if (!error_message.empty())
		{
			msg = fmt::format(fmt(FATAL_ERROR_MESSAGE L"\n\n{}\n\n{}"), message, error_message);
		}
		else
		{
			msg = fmt::format(fmt(FATAL_ERROR_MESSAGE L"\n\n{}"), message);
		}

		Log(msg, spdlog::level::critical, file, line, function);

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

#define HresultErrorHandle(__exception, __level, __message) (Error::Handle<(__level)>((__message), Error::MessageFromHresultError((__exception)), __ERROR_LOCATION))
#define HresultErrorCatch(__level, __message) catch (const winrt::hresult_error &__exception) { HresultErrorHandle(__exception, (__level), (__message)); }
