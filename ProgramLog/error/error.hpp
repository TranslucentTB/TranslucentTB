#pragma once
#include "arch.h"
#include <spdlog/common.h>
#include <source_location>
#include <string_view>
#include <thread>

#include "../api.h"
#include "appinfo.hpp"
#include "util/null_terminated_string_view.hpp"

#define UTF8_ERROR_TITLE UTF8_APP_NAME " - Error"

namespace Error {
	namespace impl {
		PROGRAMLOG_API bool ShouldLogInternal(spdlog::level::level_enum level);

		// Needs to be in DLL because spdlog log registry is per-module.
		PROGRAMLOG_API void Log(std::wstring_view msg, spdlog::level::level_enum level, std::source_location location);

		PROGRAMLOG_API std::wstring GetLogMessage(std::wstring_view message, std::wstring_view error_message);

		template<spdlog::level::level_enum level>
		inline void Handle(std::wstring_view message, std::wstring_view error_message, std::source_location location)
		{
			Log(GetLogMessage(message, error_message), level, location);
		}

		template<>
		PROGRAMLOG_API void Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, std::source_location location);

		template<>
		[[noreturn]] PROGRAMLOG_API void Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, std::source_location location);

		std::thread HandleCommon(spdlog::level::level_enum level, std::wstring_view message, std::wstring_view error_message, std::source_location location, Util::null_terminated_wstring_view title, std::wstring_view description, unsigned int type);
		void HandleCriticalCommon(std::wstring_view message, std::wstring_view error_message, std::source_location location);
	}

	template<spdlog::level::level_enum level>
	inline bool ShouldLog()
	{
		return impl::ShouldLogInternal(level);
	}

	template<>
	constexpr bool ShouldLog<spdlog::level::critical>()
	{
		return true;
	}

	template<>
	constexpr bool ShouldLog<spdlog::level::err>()
	{
		return true;
	}
};

#define PROGRAMLOG_ERROR_LOCATION std::source_location::current()
#define MessagePrint(level_, message_) Error::impl::Handle<(level_)>((message_), std::wstring_view { }, PROGRAMLOG_ERROR_LOCATION)
