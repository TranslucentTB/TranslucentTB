#pragma once
#include <cerrno>
#include <system_error>
#include <string>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API std::wstring MessageFromErrno(errno_t err);
	PROGRAMLOG_API std::wstring MessageFromStdSystemError(const std::system_error &err);
}

#define ErrnoTHandle(err_, level_, message_) (Error::HandleImpl<(level_)>::Handle((message_), Error::MessageFromErrno((err_)), PROGRAMLOG_ERROR_LOCATION))
#define StdSystemErrorHandle(exception_, level_, message_) (Error::HandleImpl<(level_)>::Handle((message_), Error::MessageFromStdSystemError((exception_)), PROGRAMLOG_ERROR_LOCATION))
#define StdSystemErrorCatch(level_, message_) catch (const std::system_error &exception_) { StdSystemErrorHandle(exception_, (level_), (message_)); }
