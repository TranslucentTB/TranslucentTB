#pragma once
#include "arch.h"
#include <roerrorapi.h>
#include <windef.h>
#include <winerror.h>
#include <winrt/base.h>
#include <wil/result_macros.h>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API std::wstring MessageFromHRESULT(HRESULT result);
	PROGRAMLOG_API std::wstring MessageFromIRestrictedErrorInfo(IRestrictedErrorInfo *info);
	PROGRAMLOG_API std::wstring MessageFromHresultError(const winrt::hresult_error &err);

	template<spdlog::level::level_enum level>
	inline void VerifyHr(HRESULT hr, std::wstring_view message, const char* file, int line, const char* function)
	{
		if (FAILED(hr))
		{
			Handle<level>(message, MessageFromHRESULT(hr), file, line, function);
		}
	}
}

#define HresultVerify(hresult_, level_, message_) (Error::VerifyHr<(level_)>((hresult_), (message_), PROGRAMLOG_ERROR_LOCATION))
#define HresultHandle(hresult_, level_, message_) (Error::Handle<(level_)>((message_), Error::MessageFromHRESULT((hresult_)), PROGRAMLOG_ERROR_LOCATION))
#define LastErrorHandle(level_, message_) (HresultHandle(HRESULT_FROM_WIN32(GetLastError()), (level_), (message_)))

#define HresultErrorHandle(exception_, level_, message_) (Error::Handle<(level_)>((message_), Error::MessageFromHresultError((exception_)), PROGRAMLOG_ERROR_LOCATION))
#define HresultErrorCatch(level_, message_) catch (const winrt::hresult_error &exception_) { HresultErrorHandle(exception_, (level_), (message_)); }

#define ResultExceptionHandle(exception_, level_, message_) (Error::Handle<(level_)>((message_), Error::MessageFromHRESULT((exception_).GetErrorCode()), PROGRAMLOG_ERROR_LOCATION))
#define ResultExceptionCatch(level_, message_) catch (const wil::ResultException &exception_) { ResultExceptionHandle(exception_, (level_), (message_)); }
