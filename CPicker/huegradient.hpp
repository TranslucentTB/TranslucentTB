#pragma once
#include <array>
#include <d2d1.h>

#include "SColour.hpp"

constexpr uint8_t HueGradientPrecision = 20; // Number of steps in the hue gradient
constexpr const std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> CalculateHueGradient()
{
	std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> gradientStops{};

	SColour tempcol{};
	tempcol.h = 359;
	tempcol.s = 100;
	tempcol.v = 100;
	tempcol.UpdateRGB();

	const float step = 359 / (float)HueGradientPrecision;

	for (uint8_t i = 0; i < HueGradientPrecision; i++)
	{
		// Due to some weird MSBuild thing, C++17 is half implemented in Clang-CL.
		// Because of that, we don't have constexpr operator[] on std::array.
		// Access the raw underlying array directly instead.
		gradientStops._Elems[i] = {
			i / (float)(HueGradientPrecision - 1),
			{
				tempcol.r / 255.0f,
				tempcol.g / 255.0f,
				tempcol.b / 255.0f,
				1.0f
			}
		};

		// Clang errors out on the use of -= here
		tempcol.h = tempcol.h - step;
		tempcol.UpdateRGB();
	}

	return gradientStops;
}

inline const auto &GetHueGradient()
{
	static constexpr auto value = CalculateHueGradient();
	return value;
}