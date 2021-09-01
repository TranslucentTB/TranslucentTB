#pragma once

#ifdef PROGRAMLOG_EXPORTS
#define PROGRAMLOG_API __declspec(dllexport)
#else
#define PROGRAMLOG_API __declspec(dllimport)
#endif
