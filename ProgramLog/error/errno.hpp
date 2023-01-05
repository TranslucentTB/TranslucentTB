#pragma once
#include <cerrno>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API std::wstring MessageFromErrno(errno_t err);
}

#define ErrnoTHandle(err_, level_, message_) ErrorHandleCommonMacro((level_), (message_), Error::MessageFromErrno((err_)))
