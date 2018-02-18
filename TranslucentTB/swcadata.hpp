#pragma once
#ifndef SWCADATA_HPP
#define SWCADATA_HPP

#include <cstdint>

namespace swca {

	enum ACCENT {									// Values passed to SetWindowCompositionAttribute determining the appearance of a window
		ACCENT_ENABLE_GRADIENT = 1,					// Use a solid color specified by nColor. This mode ignores the alpha value and is fully opaque.
		ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,		// Use a tinted transparent overlay. nColor is the tint color.
		ACCENT_ENABLE_BLURBEHIND = 3,				// Use a tinted blurry overlay. nColor is the tint color.
		ACCENT_ENABLE_FLUENT = 4,					// Use an aspect similar to Fluent design. nColor is tint color. This mode bugs if the alpha value is 0.

		ACCENT_NORMAL = 150							// (Fake value) Emulate regular taskbar appearance
	};

	enum WindowCompositionAttribute {				// Possible kinds of data sent to SetWindowCompositionAttribute
													// ...
		WCA_ACCENT_POLICY = 19						// The data sent is an ACCENTPOLICY struct
													// ...
	};

	struct ACCENTPOLICY					// Determines how a window's transparent region will be painted
	{
		ACCENT nAccentState;			// Appearance
		int nFlags;						// Nobody knows how this value works
		uint32_t nColor;				// A color in the hex format AABBGGRR
		int nAnimationId;				// Nobody knows how this value works
	};

	struct WINCOMPATTRDATA							// Composition Attributes
	{
		WindowCompositionAttribute nAttribute;		// Type of the data struct passed
		void *pData;								// Opaque pointer to the data struct (ACCENTPOLICY in our case)
		uint32_t ulDataSize;						// Size of data struct
	};

}

#endif // !SWCADATA_HPP
