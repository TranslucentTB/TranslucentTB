#pragma once
#include "../arch.h"
#include <windef.h>

typedef BOOL(WINAPI* PFN_ALLOW_DARK_MODE_FOR_APP)(bool allow);
typedef BOOL(WINAPI *PFN_ALLOW_DARK_MODE_FOR_WINDOW)(HWND window, bool allow);
typedef void(WINAPI *PFN_FLUSH_MENU_THEMES)();