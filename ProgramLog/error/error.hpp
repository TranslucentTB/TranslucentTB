#pragma once
#include "arch.h"
#include <fmt/format.h>
#include <RestrictedErrorInfo.h>
#include <spdlog/common.h>
#include <string_view>
#include <windef.h>
#include <winbase.h>
#include <wil/resource.h>

#include "../api.h"
#include "appinfo.hpp"
#include "util/memory.hpp"
#include "util/null_terminated_string_view.hpp"
#include "util/to_string_view.hpp"

#define FATAL_ERROR_MESSAGE APP_NAME L" has encountered a fatal error and cannot continue executing."
#define ERROR_MESSAGE APP_NAME L" has encountered an error."

#define FATAL_ERROR_TITLE APP_NAME L" - Fatal error"
#define UTF8_ERROR_TITLE UTF8_APP_NAME " - Error"
#define ERROR_TITLE UTIL_WIDEN(UTF8_ERROR_TITLE)

namespace Error {
	PROGRAMLOG_API bool ShouldLog(spdlog::level::level_enum level);

	namespace impl {
		struct msgbox_info : Util::flexible_array<msgbox_info> {
			Util::null_terminated_wstring_view title;
			unsigned int type;
			wchar_t body[];
		};

		// Needs to be in DLL because spdlog log registry is per-module.
		PROGRAMLOG_API void Log(const fmt::wmemory_buffer &msg, spdlog::level::level_enum level, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function);

		PROGRAMLOG_API void GetLogMessage(fmt::wmemory_buffer &out, std::wstring_view message, std::wstring_view error_message, std::wstring_view err_message_fmt = L"{} ({})", std::wstring_view message_fmt = L"{}");

		template<spdlog::level::level_enum level>
		inline void Handle(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT, IRestrictedErrorInfo*)
		{
			if (ShouldLog(level))
			{
				fmt::wmemory_buffer buf;
				GetLogMessage(buf, message, error_message);
				Log(buf, level, file, line, function);
			}
		}

		template<>
		PROGRAMLOG_API void Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT, IRestrictedErrorInfo*);

		template<>
		[[noreturn]] PROGRAMLOG_API void Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT err, IRestrictedErrorInfo *errInfo);
	}

	wil::unique_handle CreateMessageBoxThread(const fmt::wmemory_buffer &buf, Util::null_terminated_wstring_view title, unsigned int type);

	template<spdlog::level::level_enum level>
	struct HandleImpl {
		template<typename T, typename U>
		requires Util::is_convertible_to_wstring_view_v<T> && Util::is_convertible_to_wstring_view_v<U>
		inline static void Handle(const T &message, const U &error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT err = E_FAIL, IRestrictedErrorInfo *errInfo = nullptr)
		{
			impl::Handle<level>(Util::ToStringView(message), Util::ToStringView(error_message), file, line, function, err, errInfo);
		}
	};

	template<>
	struct HandleImpl<spdlog::level::critical> {
		template<typename T, typename U>
#ifdef __cpp_concepts // MIGRATION: IDE concept support
			requires Util::is_convertible_to_wstring_view_v<T> && Util::is_convertible_to_wstring_view_v<U>
#endif
		[[noreturn]] inline static void Handle(const T &message, const U &error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function, HRESULT err = E_FAIL, IRestrictedErrorInfo *errInfo = nullptr)
		{
			impl::Handle<spdlog::level::critical>(Util::ToStringView(message), Util::ToStringView(error_message), file, line, function, err, errInfo);
		}
	};
};

template<>
struct Util::flexible_array_traits<Error::impl::msgbox_info> {
	static constexpr auto data = &Error::impl::msgbox_info::body;
};

#define PROGRAMLOG_ERROR_LOCATION __FILE__, __LINE__, SPDLOG_FUNCTION
#define MessagePrint(level_, message_) Error::HandleImpl<(level_)>::Handle((message_), std::wstring_view { }, PROGRAMLOG_ERROR_LOCATION)
