#pragma once
#include <rapidjson/error/en.h>

#include "error.hpp"

#define ParseErrorCodeHandle(code_, level_, message_) (Error::HandleImpl<(level_)>::Handle((message_), rapidjson::GetParseError_En((code_)), PROGRAMLOG_ERROR_LOCATION))
