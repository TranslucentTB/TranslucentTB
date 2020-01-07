#pragma once
#include <cerrno>
#include <system_error>
#include <string>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API void MessageFromErrno(fmt::wmemory_buffer &buf, errno_t err);
	PROGRAMLOG_API void MessageFromStdSystemError(fmt::wmemory_buffer &buf, const std::system_error &err);
}

#define ErrnoTHandle(err_, level_, message_) do { \
	fmt::wmemory_buffer buf_; \
	Error::MessageFromErrno(buf_, (err_)); \
	Error::HandleImpl<(level_)>::Handle((message_), buf_, PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define StdSystemErrorHandle(exception_, level_, message_) do { \
	fmt::wmemory_buffer buf_; \
	Error::MessageFromStdSystemError(buf_, (exception_)); \
	Error::HandleImpl<(level_)>::Handle((message_), buf_, PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define StdSystemErrorCatch(level_, message_) catch (const std::system_error &exception_) { StdSystemErrorHandle(exception_, (level_), (message_)); }
