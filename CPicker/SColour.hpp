#pragma once
#include <cstdint>

struct SColour
{
	// Red, green and blue
	uint8_t r, g, b;

	// Hue, saturation and value
	unsigned short h;
	uint8_t s, v;

	// Alpha
	uint8_t a;

	_declspec(dllexport) void UpdateRGB();			// Updates RGB from HSV
	_declspec(dllexport) void UpdateHSV();			// Updates HSV from RGB
};