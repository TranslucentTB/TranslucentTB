#pragma once
#include "arch.h"
#include <restrictederrorinfo.h>
#include <windef.h>
#include <winerror.h>
#include "../Common/winrt.hpp"

#include "error.hpp"

namespace Error {
	namespace impl {
		std::wstring FormatIRestrictedErrorInfo(HRESULT result, BSTR description);

		[[noreturn]] PROGRAMLOG_API void HandleCriticalWithErrorInfo(std::wstring_view message, std::wstring_view error_message, std::source_location location, HRESULT err, IRestrictedErrorInfo* errInfo);
	}

	PROGRAMLOG_API std::wstring MessageFromIRestrictedErrorInfo(IRestrictedErrorInfo *info, HRESULT errCode);
	PROGRAMLOG_API std::wstring MessageFromHresultError(const winrt::hresult_error &error);
}

#define HresultErrorHandle(exception_, level_, message_) do { \
	const winrt::hresult_error &hresultError_ = (exception_); \
	if constexpr ((level_) == spdlog::level::critical) \
	{ \
		Error::impl::HandleCriticalWithErrorInfo((message_), Error::MessageFromHresultError(hresultError_), PROGRAMLOG_ERROR_LOCATION, hresultError_.code(), hresultError_.try_as<IRestrictedErrorInfo>().get()); \
	} \
	else if (Error::ShouldLog<(level_)>()) \
	{ \
		Error::impl::Handle<(level_)>((message_), Error::MessageFromHresultError(hresultError_), PROGRAMLOG_ERROR_LOCATION); \
	} \
} while (0)

#define HresultErrorCatch(level_, message_) catch (const winrt::hresult_error &exception_) { HresultErrorHandle(exception_, (level_), (message_)); }
