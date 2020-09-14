#pragma once
#include <system_error>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API void MessageFromStdErrorCode(fmt::wmemory_buffer &buf, const std::error_code &err);
}

#define StdErrorCodeHandleWithBuffer(buf_, errc_, level_, message_) do { \
	fmt::wmemory_buffer &bufLocal_ = (buf_); \
	Error::MessageFromStdErrorCode(bufLocal_, (errc_)); \
	Error::impl::Handle<(level_)>(Util::ToStringView((message_)), Util::ToStringView(bufLocal_), PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define StdErrorCodeHandle(errc_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		fmt::wmemory_buffer buf_; \
		StdErrorCodeHandleWithBuffer(buf_, (errc_), (level_), (message_)); \
	} \
} while (0)

#define StdErrorCodeVerify(errc_, level_, message_) do { \
	if (errc_) \
	{ \
		StdErrorCodeHandle(errc_, (level_), (message_)); \
	} \
} while (0)

#define StdSystemErrorHandleWithBuffer(buf_, exception_, level_, message_) StdErrorCodeHandleWithBuffer((buf_), (exception_).code(), (level_), (message_))

#define StdSystemErrorHandle(exception_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		fmt::wmemory_buffer buf_; \
		StdSystemErrorHandleWithBuffer(buf_, (exception_), (level_), (message_)); \
	} \
} while (0)

#define StdSystemErrorCatch(level_, message_) catch (const std::system_error &exception_) { StdSystemErrorHandle(exception_, (level_), (message_)); }
