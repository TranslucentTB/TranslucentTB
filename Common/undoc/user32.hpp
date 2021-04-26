#pragma once
#include "../arch.h"
#include <windef.h>

// Enum           : ACCENT_STATE, Type: int
// Data           :   constant 0x0, Constant, Type: int, ACCENT_DISABLED
// Data           :   constant 0x1, Constant, Type: int, ACCENT_ENABLE_GRADIENT
// Data           :   constant 0x2, Constant, Type: int, ACCENT_ENABLE_TRANSPARENTGRADIENT
// Data           :   constant 0x3, Constant, Type: int, ACCENT_ENABLE_BLURBEHIND
// Data           :   constant 0x4, Constant, Type: int, ACCENT_ENABLE_ACRYLICBLURBEHIND
// Data           :   constant 0x5, Constant, Type: int, ACCENT_ENABLE_HOSTBACKDROP
// Data           :   constant 0x6, Constant, Type: int, ACCENT_INVALID_STATE
enum ACCENT_STATE : INT {				// Affects the rendering of the background of a window.
	ACCENT_DISABLED = 0,					// Default value. Background is black.
	ACCENT_ENABLE_GRADIENT = 1,				// Background is GradientColor, alpha channel ignored.
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,	// Background is GradientColor.
	ACCENT_ENABLE_BLURBEHIND = 3,			// Background is GradientColor, with blur effect.
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,	// Background is GradientColor, with acrylic blur effect.
	ACCENT_ENABLE_HOSTBACKDROP = 5,			// Unknown.
	ACCENT_INVALID_STATE = 6,				// Unknown. Seems to draw background fully transparent.

	ACCENT_NORMAL = 0x0						// Fake value: tells TTB to send a message to the taskbar for it restore its default effect.
};

// UserDefinedType: ACCENT_POLICY
// Data           :   this+0x0, Member, Type: enum ACCENT_STATE, AccentState
// Data           :   this+0x4, Member, Type: unsigned int, AccentFlags
// Data           :   this+0x8, Member, Type: unsigned long, GradientColor
// Data           :   this+0xC, Member, Type: long, AnimationId
struct ACCENT_POLICY {			// Determines how a window's background is rendered.
	ACCENT_STATE	AccentState;	// Background effect.
	UINT			AccentFlags;	// Flags. Set to 2 to tell GradientColor is used, rest is unknown.
	COLORREF		GradientColor;	// Background color.
	LONG			AnimationId;	// Unknown
};

// Enum           : WINDOWCOMPOSITIONATTRIB, Type: int
// Data           :   constant 0x0, Constant, Type: int, WCA_UNDEFINED
// Data           :   constant 0x1, Constant, Type: int, WCA_NCRENDERING_ENABLED
// Data           :   constant 0x2, Constant, Type: int, WCA_NCRENDERING_POLICY
// ...
// Data           :   constant 0x13, Constant, Type: int, WCA_ACCENT_POLICY
// ...
// Data           :   constant 0x1A, Constant, Type: int, WCA_LAST
enum WINDOWCOMPOSITIONATTRIB : INT {	// Determines what attribute is being manipulated.
	WCA_ACCENT_POLICY = 0x13				// The attribute being get or set is an accent policy.
};

// UserDefinedType: tagWINDOWCOMPOSITIONATTRIBDATA
// Data           :   this+0x0, Member, Type: enum WINDOWCOMPOSITIONATTRIB, Attrib
// Data           :   this+0x8, Member, Type: void *, pvData
// Data           :   this+0x10, Member, Type: unsigned int, cbData
struct WINDOWCOMPOSITIONATTRIBDATA {	// Options for [Get/Set]WindowCompositionAttribute.
	WINDOWCOMPOSITIONATTRIB	Attrib;			// Type of what is being get or set.
	LPVOID					pvData;			// Pointer to memory that will receive what is get or that contains what will be set.
	UINT					cbData;			// Size of the data being pointed to by pvData.
};

typedef BOOL (WINAPI* PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE)(HWND, const WINDOWCOMPOSITIONATTRIBDATA *);
