#pragma once
#include <rapidjson/error/en.h>

#include "error.hpp"

#define ParseErrorCodeHandle(code_, level_, message_) (Error::Handle<(level_)>((message_), rapidjson::GetParseError_En((code_)), PROGRAMLOG_ERROR_LOCATION))
