#pragma once
#include <system_error>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API std::wstring MessageFromStdErrorCode(const std::error_code &err);
}

#define StdErrorCodeHandle(errc_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		Error::impl::Handle<(level_)>((message_), Error::MessageFromStdErrorCode((errc_)), PROGRAMLOG_ERROR_LOCATION); \
	} \
} while (0)

#define StdErrorCodeVerify(errc_, level_, message_) do { \
	if (const std::error_code ec_ = (errc_); ec_) [[unlikely]] \
	{ \
		StdErrorCodeHandle(ec_, (level_), (message_)); \
	} \
} while (0)

#define StdSystemErrorHandle(exception_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		Error::impl::Handle<(level_)>((message_), Error::MessageFromStdErrorCode((exception_).code()), PROGRAMLOG_ERROR_LOCATION); \
	} \
} while (0)

#define StdSystemErrorCatch(level_, message_) catch (const std::system_error &exception_) { StdSystemErrorHandle(exception_, (level_), (message_)); }
