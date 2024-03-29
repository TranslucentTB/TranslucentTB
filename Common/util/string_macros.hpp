#pragma once
#define UTIL_WIDEN_INNER(x) L##x
#define UTIL_WIDEN(x) UTIL_WIDEN_INNER(x)

#define UTIL_STRINGIFY_INNER(x) #x
#define UTIL_STRINGIFY(x) UTIL_WIDEN(UTIL_STRINGIFY_INNER(x))
#define UTIL_STRINGIFY_UTF8(x) UTIL_STRINGIFY_INNER(x)
