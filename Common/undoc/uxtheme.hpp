#pragma once
#include "../arch.h"
#include <windef.h>

// Enum           : PreferredAppMode, Type: int
// Data           :   constant 0x0, Constant, Type: int, Default
// Data           :   constant 0x1, Constant, Type: int, AllowDark
// Data           :   constant 0x2, Constant, Type: int, ForceDark
// Data           :   constant 0x3, Constant, Type: int, ForceLight
// Data           :   constant 0x4, Constant, Type: int, Max
enum class PreferredAppMode : INT {
	Default = 0,
	AllowDark = 1,
	ForceDark = 2,
	ForceLight = 3,
	Max = 4
};

typedef PreferredAppMode(WINAPI* PFN_SET_PREFERRED_APP_MODE)(PreferredAppMode appMode);
typedef BOOL(WINAPI* PFN_ALLOW_DARK_MODE_FOR_WINDOW)(HWND window, bool allow);
typedef BOOL(WINAPI* PFN_SHOULD_APPS_USE_DARK_MODE)();
typedef BOOL(WINAPI* PFN_SHOULD_SYSTEM_USE_DARK_MODE)();
