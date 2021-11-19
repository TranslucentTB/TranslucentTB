#pragma once
#include "arch.h"
#include <RestrictedErrorInfo.h>
#include <spdlog/common.h>
#include <string_view>
#include <thread>
#include <winerror.h>

#include "../api.h"
#include "appinfo.hpp"
#include "util/null_terminated_string_view.hpp"

#define FATAL_ERROR_MESSAGE APP_NAME L" has encountered a fatal error and cannot continue executing."
#define ERROR_MESSAGE APP_NAME L" has encountered an error."

#define FATAL_ERROR_TITLE APP_NAME L" - Fatal error"
#define UTF8_ERROR_TITLE UTF8_APP_NAME " - Error"
#define ERROR_TITLE UTIL_WIDEN(UTF8_ERROR_TITLE)

namespace Error {
	PROGRAMLOG_API bool ShouldLog(spdlog::level::level_enum level);

	namespace impl {
		// Needs to be in DLL because spdlog log registry is per-module.
		PROGRAMLOG_API void Log(std::wstring_view msg, spdlog::level::level_enum level, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function);

		PROGRAMLOG_API std::wstring GetLogMessage(std::wstring_view message, std::wstring_view error_message, std::wstring_view err_message_fmt = L"{} ({})", std::wstring_view message_fmt = L"{}");

		template<spdlog::level::level_enum level>
		inline void Handle(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT = E_FAIL, IRestrictedErrorInfo* = nullptr)
		{
			if (ShouldLog(level))
			{
				Log(GetLogMessage(message, error_message), level, file, line, function);
			}
		}

		template<>
		PROGRAMLOG_API void Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT, IRestrictedErrorInfo*);

		template<>
		[[noreturn]] PROGRAMLOG_API void Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT err, IRestrictedErrorInfo *errInfo);

		std::thread CreateMessageBoxThread(std::wstring buf, Util::null_terminated_wstring_view title, unsigned int type);
	}
};

#define PROGRAMLOG_ERROR_LOCATION __FILE__, __LINE__, SPDLOG_FUNCTION
#define MessagePrint(level_, message_) Error::impl::Handle<(level_)>((message_), std::wstring_view { }, PROGRAMLOG_ERROR_LOCATION)
