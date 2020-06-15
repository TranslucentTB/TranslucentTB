#pragma once
#include <cerrno>
#include <system_error>
#include <string>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API void MessageFromErrno(fmt::wmemory_buffer &buf, errno_t err);
	PROGRAMLOG_API void MessageFromStdSystemError(fmt::wmemory_buffer &buf, const std::system_error &err);
}

#define ErrnoTHandleWithBuffer(buf_, err_, level_, message_) do { \
	fmt::wmemory_buffer &bufLocal_ = (buf_); \
	Error::MessageFromErrno(bufLocal_, (err_)); \
	Error::HandleImpl<(level_)>::Handle((message_), bufLocal_, PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define ErrnoTHandle(err_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		fmt::wmemory_buffer buf_; \
		ErrnoTHandleWithBuffer(buf_, (err_), (level_), (message_)); \
	} \
} while (0)

#define ErrnoHandle(level_, message_) ErrnoTHandle(errno, (level_), (message_))

#define StdSystemErrorHandleWithBuffer(buf_, exception_, level_, message_) do { \
	fmt::wmemory_buffer &bufLocal_ = (buf_); \
	Error::MessageFromStdSystemError(bufLocal_, (exception_)); \
	Error::HandleImpl<(level_)>::Handle((message_), bufLocal_, PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define StdSystemErrorHandle(exception_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		fmt::wmemory_buffer buf_; \
		StdSystemErrorHandleWithBuffer(buf_, (exception_), (level_), (message_)); \
	} \
} while (0)

#define StdSystemErrorCatch(level_, message_) catch (const std::system_error &exception_) { StdSystemErrorHandle(exception_, (level_), (message_)); }
