#pragma once
#ifndef COMPOSITIONDATA_H
#define COMPISITIONDATA_H

#include <cstdint>

enum ACCENTSTATE {								// Values passed to SetWindowCompositionAttribute determining the appearance of a window
	ACCENT_ENABLE_GRADIENT = 1,					// Use a solid color specified by nColor. This mode doesn't care about the alpha channel.
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,		// Use a tinted transparent overlay. nColor is the tint color, sending nothing results in it interpreted as 0x00000000 (totally transparent, blends in with desktop)
	ACCENT_ENABLE_BLURBEHIND = 3,				// Use a tinted blurry overlay. nColor is the tint color, sending nothing results in it interpreted as 0x00000000 (totally transparent, blends in with desktop)
	ACCENT_ENABLE_FLUENT = 4,					// Use fluent design-like aspect. nColor is tint color.

	ACCENT_FOLLOW_OPT = 149,					// (Fake value) Use the value in opt.taskbar_appearance
	ACCENT_ENABLE_TINTED = 150,					// (Fake value) Dynamic windows tinted
	ACCENT_NORMAL = 151							// (Fake value) Emulate regular taskbar appearance
};

enum WindowCompositionAttribute {				// Possible kinds of data sent to SetWindowCompositionAttribute
												// ...
	WCA_ACCENT_POLICY = 19						// The data sent is an ACCENTPOLICY struct
												// ...
};

struct ACCENTPOLICY					// Determines how a window's transparent region will be painted
{
	ACCENTSTATE nAccentState;		// Appearance
	int nFlags;						// Nobody knows how this value works
	uint32_t nColor;					// A color in the hex format AABBGGRR
	int nAnimationId;				// Nobody knows how this value works
};

struct WINCOMPATTRDATA							// Composition Attributes
{
	WindowCompositionAttribute nAttribute;		// Type of the data passed in nAttribute
	void *pData;								// Opaque pointer to the data struct (ACCENTPOLICY)
	ULONG ulDataSize;
};

#endif // !COMPOSITIONDATA_H
