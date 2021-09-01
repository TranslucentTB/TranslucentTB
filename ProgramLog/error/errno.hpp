#pragma once
#include <cerrno>

#include "error.hpp"

namespace Error {
	PROGRAMLOG_API void MessageFromErrno(fmt::wmemory_buffer &buf, errno_t err);
}

#define ErrnoTHandleWithBuffer(buf_, err_, level_, message_) do { \
	fmt::wmemory_buffer &bufLocal_ = (buf_); \
	Error::MessageFromErrno(bufLocal_, (err_)); \
	Error::impl::Handle<(level_)>(Util::ToStringView((message_)), Util::ToStringView(bufLocal_), PROGRAMLOG_ERROR_LOCATION); \
} while (0)

#define ErrnoTHandle(err_, level_, message_) do { \
	if (Error::ShouldLog((level_))) \
	{ \
		fmt::wmemory_buffer buf_; \
		ErrnoTHandleWithBuffer(buf_, (err_), (level_), (message_)); \
	} \
} while (0)
