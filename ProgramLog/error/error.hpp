#pragma once
#include <spdlog/common.h>
#include <string>
#include <string_view>

#include "../api.h"
#include "appinfo.hpp"

#define FATAL_ERROR_MESSAGE APP_NAME L" has encountered a fatal error and cannot continue executing."
#define ERROR_MESSAGE APP_NAME L" has encountered an error."

#define FATAL_ERROR_TITLE APP_NAME L" - Fatal error"
#define UTF8_ERROR_TITLE UTF8_APP_NAME " - Error"
#define ERROR_TITLE UTIL_WIDEN(UTF8_ERROR_TITLE)

namespace Error {
	namespace impl {
		// Needs to be in DLL because spdlog log registry is per-module.
		PROGRAMLOG_API void Log(std::wstring_view msg, spdlog::level::level_enum level, const char *file, int line, const char *function);

		PROGRAMLOG_API std::wstring GetLogMessage(std::wstring_view message, std::wstring_view error_message, std::wstring_view err_message_fmt = L"{} ({})", std::wstring_view message_fmt = L"{}");
	}

	template<spdlog::level::level_enum level>
	inline void Handle(std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function)
	{
		impl::Log(impl::GetLogMessage(message, error_message), level, file, line, function);
	}

	template<>
	PROGRAMLOG_API void Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function);

	template<>
	[[noreturn]] PROGRAMLOG_API void Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, const char* file, int line, const char* function);
};

#define PROGRAMLOG_ERROR_LOCATION __FILE__, __LINE__, SPDLOG_FUNCTION
#define MessagePrint(level_, message_) (Error::Handle<(level_)>((message_), { }, PROGRAMLOG_ERROR_LOCATION))
