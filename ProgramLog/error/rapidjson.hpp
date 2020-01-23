#pragma once
#include <rapidjson/error/en.h>

#include "error.hpp"
#include "../../Common/config/rapidjsonhelper.hpp"

#define ParseErrorCodeHandle(code_, level_, message_) Error::HandleImpl<(level_)>::Handle((message_), rapidjson::GetParseError_En((code_)), PROGRAMLOG_ERROR_LOCATION)

#define HelperDeserializationErrorHandle(exception_, level_, message_) Error::HandleImpl<(level_)>::Handle((message_), (exception_).what, PROGRAMLOG_ERROR_LOCATION)
#define HelperDeserializationErrorCatch(level_, message_) catch (const RapidJSONHelper::DeserializationError &exception_) { HelperDeserializationErrorHandle(exception_, (level_), (message_)); }
