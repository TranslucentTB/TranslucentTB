#pragma once
#include "util/string_macros.hpp"

#if defined(BUILD_TYPE) && BUILD_TYPE == 0
#define UTF8_APP_NAME "TranslucentTB"
#elif defined(BUILD_TYPE) && BUILD_TYPE == 1
#define UTF8_APP_NAME "TranslucentTB (Canary)"
#else
#define UTF8_APP_NAME "TranslucentTB (Dev)"
#endif

#define APP_NAME UTIL_WIDEN(UTF8_APP_NAME)

#define APP_COPYRIGHT_YEAR_NUM 2024
#define APP_COPYRIGHT_YEAR UTIL_STRINGIFY(APP_COPYRIGHT_YEAR_NUM)

#define APP_VERSION_FIXED 1,0,0,1
#define APP_VERSION UTIL_WIDEN("1.0.0.1")
