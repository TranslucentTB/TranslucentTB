#pragma once
#include "arch.h"
#include <windef.h>
#include <winerror.h>
#include <wil/result_macros.h>
#include <wil/resource.h>

#include "error.hpp"

namespace Error {
	namespace impl {
		std::wstring FormatHRESULT(HRESULT result, std::wstring_view description);
		wil::unique_hlocal_string FormatMessageForLanguage(HRESULT result, DWORD langId, DWORD &count);
	}

	PROGRAMLOG_API std::wstring MessageFromHRESULT(HRESULT result);
}

#define HresultHandle(hresult_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		Error::impl::Handle<(level_)>((message_), Error::MessageFromHRESULT((hresult_)), PROGRAMLOG_ERROR_LOCATION); \
	} \
} while (0)

#define HresultVerify(hresult_, level_, message_) do { \
	if (const HRESULT hr_ = (hresult_); FAILED(hr_)) [[unlikely]] \
	{ \
		HresultHandle(hr_, (level_), (message_)); \
	} \
} while (0)

#define LastErrorHandle(level_, message_) do { \
	const HRESULT hr_ = HRESULT_FROM_WIN32(GetLastError()); \
	HresultHandle(hr_, (level_), (message_)); \
} while (0)

#define LastErrorVerify(level_, message_) do { \
	if (const DWORD lastErr_ = GetLastError(); lastErr_ != NO_ERROR) [[unlikely]] \
	{ \
		HresultHandle(HRESULT_FROM_WIN32(lastErr_), (level_), (message_)); \
	} \
} while (0)

#define ResultExceptionHandle(exception_, level_, message_) HresultHandle((exception_).GetErrorCode(), (level_), (message_))

#define ResultExceptionCatch(level_, message_) catch (const wil::ResultException &exception_) { ResultExceptionHandle(exception_, (level_), (message_)); }
