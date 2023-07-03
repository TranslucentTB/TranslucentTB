#pragma once
#include <rapidjson/error/en.h>

#include "error.hpp"
#include "../../Common/config/rapidjsonhelper.hpp"

#define ParseErrorCodeHandle(code_, level_, message_) ErrorHandleCommonMacro((level_), (message_), rj::GetParseError_En((code_)))

#define HelperDeserializationErrorHandle(exception_, level_, message_) ErrorHandleCommonMacro((level_), (message_), (exception_).what)

#define HelperDeserializationErrorCatch(level_, message_) catch (const rjh::DeserializationError &exception_) { HelperDeserializationErrorHandle(exception_, (level_), (message_)); }
