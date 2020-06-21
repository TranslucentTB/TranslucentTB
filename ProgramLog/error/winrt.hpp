#pragma once
#include "arch.h"
#include <restrictederrorinfo.h>
#include <windef.h>
#include "../Common/winrt.hpp"

#include "error.hpp"

namespace Error {
#ifdef PROGRAMLOG_EXPORTS
	namespace impl {
		void FormatIRestrictedErrorInfo(fmt::wmemory_buffer &buf, HRESULT result, BSTR description);
	}
#endif

	PROGRAMLOG_API bool MessageFromIRestrictedErrorInfo(fmt::wmemory_buffer &buf, IRestrictedErrorInfo *info, HRESULT failureCode);
	PROGRAMLOG_API winrt::com_ptr<IRestrictedErrorInfo> MessageFromHresultError(fmt::wmemory_buffer &buf, const winrt::hresult_error &err, HRESULT *errCode = nullptr);
}

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
