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
	PROGRAMLOG_API bool MessageFromIRestrictedErrorInfo(fmt::wmemory_buffer &buf, IRestrictedErrorInfo *info, HRESULT failureCode);

	inline winrt::com_ptr<IRestrictedErrorInfo> MessageFromHresultError(fmt::wmemory_buffer &buf, const winrt::hresult_error &err, HRESULT *errCode = nullptr)
	{
		HRESULT code = err.code();
		if (errCode)
		{
			*errCode = code;
		}

		if (const auto info = err.try_as<IRestrictedErrorInfo>())
		{
			if (MessageFromIRestrictedErrorInfo(buf, info.get(), code))
			{
				return info;
			}
		}
		else
		{
			MessageFromHRESULT(buf, code);
		}

		return nullptr;
	}
}

#define HresultHandleWithBuffer(buf_, hresult_, level_, message_) do { \
	fmt::wmemory_buffer &bufLocal_ = (buf_); \
	Error::MessageFromHRESULT(bufLocal_, (hresult_)); \
	Error::HandleImpl<(level_)>::Handle((message_), bufLocal_, PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define HresultHandle(hresult_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		fmt::wmemory_buffer buf_; \
		HresultHandleWithBuffer(buf_, (hresult_), (level_), (message_)); \
	} \
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

#define HresultErrorHandleWithBuffer(buf_, exception_, level_, message_) do { \
	fmt::wmemory_buffer &bufLocal_ = (buf_); \
	HRESULT errCode_ { }; \
	const auto errInfo_ = Error::MessageFromHresultError(bufLocal_, (exception_), &errCode_); \
	Error::HandleImpl<(level_)>::Handle((message_), bufLocal_, PROGRAMLOG_ERROR_LOCATION, errCode_, errInfo_.get()); \
} while (0)

#define HresultErrorHandle(exception_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		fmt::wmemory_buffer buf_; \
		HresultErrorHandleWithBuffer(buf_, (exception_), (level_), (message_)); \
	} \
} while (0)

#define HresultErrorCatch(level_, message_) catch (const winrt::hresult_error &exception_) { HresultErrorHandle(exception_, (level_), (message_)); }

#define ResultExceptionHandle(exception_, level_, message_) HresultHandle((exception_).GetErrorCode(), (level_), (message_))

#define ResultExceptionCatch(level_, message_) catch (const wil::ResultException &exception_) { ResultExceptionHandle(exception_, (level_), (message_)); }
