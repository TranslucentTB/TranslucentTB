#pragma once
#include "arch.h"
#include <restrictederrorinfo.h>
#include <windef.h>
#include <winerror.h>
#include "winrt.hpp"
#include <wil/result_macros.h>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API void MessageFromHRESULT(fmt::wmemory_buffer &buf, HRESULT result);
	PROGRAMLOG_API void MessageFromIRestrictedErrorInfo(fmt::wmemory_buffer &buf, IRestrictedErrorInfo *info);

	inline void MessageFromHresultError(fmt::wmemory_buffer &buf, const winrt::hresult_error &err)
	{
		if (const auto info = err.try_as<IRestrictedErrorInfo>())
		{
			MessageFromIRestrictedErrorInfo(buf, info.get());
		}
		else
		{
			MessageFromHRESULT(buf, err.code());
		}
	}
}

#define HresultHandle(hresult_, level_, message_) do { \
	fmt::wmemory_buffer buf_; \
	Error::MessageFromHRESULT(buf_, (hresult_)); \
	Error::HandleImpl<(level_)>::Handle((message_), buf_, PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define HresultVerify(hresult_, level_, message_) do { \
	if (const HRESULT hr_ = (hresult_); FAILED(hr_)) \
	{ \
		HresultHandle(hr_, (level_), (message_)); \
	} \
} while (0)

#define LastErrorHandle(level_, message_) HresultHandle(HRESULT_FROM_WIN32(GetLastError()), (level_), (message_))

#define LastErrorVerify(level_, message_) do { \
	if (const DWORD lastErr_ = GetLastError(); lastErr_ != NO_ERROR) \
	{ \
		HresultHandle(HRESULT_FROM_WIN32(lastErr_), (level_), (message_)); \
	} \
} while (0)

#define HresultErrorHandle(exception_, level_, message_) do { \
	fmt::wmemory_buffer buf_; \
	Error::MessageFromHresultError(buf_, (exception_)); \
	Error::HandleImpl<(level_)>::Handle((message_), buf_, PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define HresultErrorCatch(level_, message_) catch (const winrt::hresult_error &exception_) { HresultErrorHandle(exception_, (level_), (message_)); }

#define ResultExceptionHandle(exception_, level_, message_) HresultHandle((exception_).GetErrorCode(), (level_), (message_))

#define ResultExceptionCatch(level_, message_) catch (const wil::ResultException &exception_) { ResultExceptionHandle(exception_, (level_), (message_)); }
