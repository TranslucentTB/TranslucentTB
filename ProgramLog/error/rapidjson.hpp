#pragma once
#include <rapidjson/error/en.h>

#include "error.hpp"
#include "../../Common/config/rapidjsonhelper.hpp"

#define ParseErrorCodeHandle(code_, level_, message_) do { \
	if (Error::ShouldLog<(level_)>()) \
	{ \
		 Error::impl::Handle<(level_)>((message_), rj::GetParseError_En((code_)), PROGRAMLOG_ERROR_LOCATION); \
	} \
} while (0)

#define HelperDeserializationErrorHandle(exception_, level_, message_) do { \
	if (Error::ShouldLog<(level_)>()) \
	{ \
		 Error::impl::Handle<(level_)>((message_), (exception_).what, PROGRAMLOG_ERROR_LOCATION); \
	} \
} while (0)

#define HelperDeserializationErrorCatch(level_, message_) catch (const rjh::DeserializationError &exception_) { HelperDeserializationErrorHandle(exception_, (level_), (message_)); }
