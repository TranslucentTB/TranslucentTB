#pragma once
#include "arch.h"
#include <restrictederrorinfo.h>
#include <windef.h>
#include "../Common/winrt.hpp"

#include "error.hpp"

namespace Error {
	namespace impl {
		std::wstring FormatIRestrictedErrorInfo(HRESULT result, BSTR description);
	}

	PROGRAMLOG_API bool MessageFromIRestrictedErrorInfo(std::wstring &buf, IRestrictedErrorInfo *info, HRESULT failureCode);
	PROGRAMLOG_API winrt::com_ptr<IRestrictedErrorInfo> MessageFromHresultError(std::wstring &buf, const winrt::hresult_error &err, HRESULT *errCode = nullptr);
}

#define HresultErrorHandle(exception_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		std::wstring buf_; \
		HRESULT errCode_ { }; \
		const auto errInfo_ = Error::MessageFromHresultError(buf_, (exception_), &errCode_); \
		Error::impl::Handle<(level_)>((message_), buf_, PROGRAMLOG_ERROR_LOCATION, errCode_, errInfo_.get()); \
	} \
} while (0)

#define HresultErrorCatch(level_, message_) catch (const winrt::hresult_error &exception_) { HresultErrorHandle(exception_, (level_), (message_)); }
