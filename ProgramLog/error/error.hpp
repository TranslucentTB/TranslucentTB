#pragma once
#include "arch.h"
#include <RestrictedErrorInfo.h>
#include <spdlog/common.h>
#include <source_location>
#include <string_view>
#include <thread>
#include <winerror.h>

#include "../api.h"
#include "appinfo.hpp"
#include "util/null_terminated_string_view.hpp"

#define UTF8_ERROR_TITLE UTF8_APP_NAME " - Error"

namespace Error {
	PROGRAMLOG_API bool ShouldLog(spdlog::level::level_enum level);

	namespace impl {
		// Needs to be in DLL because spdlog log registry is per-module.
		PROGRAMLOG_API void Log(std::wstring_view msg, spdlog::level::level_enum level, std::source_location location);

		PROGRAMLOG_API std::wstring GetLogMessage(std::wstring_view message, std::wstring_view error_message);

		template<spdlog::level::level_enum level>
		inline void Handle(std::wstring_view message, std::wstring_view error_message, std::source_location location, HRESULT = E_FAIL, IRestrictedErrorInfo* = nullptr)
		{
			if (ShouldLog(level))
			{
				Log(GetLogMessage(message, error_message), level, location);
			}
		}

		template<>
		PROGRAMLOG_API void Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, std::source_location location, HRESULT, IRestrictedErrorInfo*);

		template<>
		[[noreturn]] PROGRAMLOG_API void Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, std::source_location location, HRESULT err, IRestrictedErrorInfo *errInfo);

		std::thread CreateMessageBoxThread(std::wstring buf, Util::null_terminated_wstring_view title, unsigned int type);
	}
};

#define PROGRAMLOG_ERROR_LOCATION std::source_location::current()
#define MessagePrint(level_, message_) Error::impl::Handle<(level_)>((message_), std::wstring_view { }, PROGRAMLOG_ERROR_LOCATION)
