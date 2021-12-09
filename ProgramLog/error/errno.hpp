#pragma once
#include <cerrno>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API std::wstring MessageFromErrno(errno_t err);
}

#define ErrnoTHandle(err_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		Error::impl::Handle<(level_)>((message_), Error::MessageFromErrno((err_)), PROGRAMLOG_ERROR_LOCATION); \
	} \
} while (0)
