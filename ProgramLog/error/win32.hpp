#pragma once
#include "arch.h"
#include <windef.h>
#include <winerror.h>
#include <wil/result_macros.h>

#include "error.hpp"

namespace Error {
	namespace impl {
		void FormatHRESULT(fmt::wmemory_buffer &buf, HRESULT result, std::wstring_view description);
	}

	PROGRAMLOG_API void MessageFromHRESULT(fmt::wmemory_buffer &buf, HRESULT result);
}

#define HresultHandleWithBuffer(buf_, hresult_, level_, message_) do { \
	fmt::wmemory_buffer &bufLocal_ = (buf_); \
	Error::MessageFromHRESULT(bufLocal_, (hresult_)); \
	Error::impl::Handle<(level_)>(Util::ToStringView((message_)), Util::ToStringView(bufLocal_), PROGRAMLOG_ERROR_LOCATION); \
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

#define ResultExceptionHandle(exception_, level_, message_) HresultHandle((exception_).GetErrorCode(), (level_), (message_))

#define ResultExceptionCatch(level_, message_) catch (const wil::ResultException &exception_) { ResultExceptionHandle(exception_, (level_), (message_)); }
