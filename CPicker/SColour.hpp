#pragma once
#include <cstdint>

#include "extern.h"

struct EXTERN SColour {

	// Red, green and blue
	uint8_t r, g, b;

	// Hue, saturation and value
	unsigned short h;
	uint8_t s, v;

	// Alpha
	uint8_t a;

	void UpdateRGB();			// Updates RGB from HSV
	void UpdateHSV();			// Updates HSV from RGB
};

#undef EXTERN