#pragma once
#include <array>
#include <d2d1.h>

#include "scolour.hpp"

constexpr uint8_t HueGradientPrecision = 20; // Number of steps in the hue gradient
constexpr const std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> CalculateHueGradient()
{
	std::array<D2D1_GRADIENT_STOP, HueGradientPrecision> gradientStops{};

	SColour tempcol{};
	tempcol.h = 359;
	tempcol.s = 100;
	tempcol.v = 100;
	tempcol.UpdateRGB();

	const float step = 359 / static_cast<float>(HueGradientPrecision);

	for (uint8_t i = 0; i < HueGradientPrecision; i++)
	{
		gradientStops[i] = {
			i / static_cast<float>(HueGradientPrecision - 1),
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

inline HRESULT CreateHueGradient(ID2D1RenderTarget *target, ID2D1LinearGradientBrush **gradient, bool vertical = false)
{
	static constexpr auto value = CalculateHueGradient();

	ID2D1GradientStopCollection *gradientStops;
	HRESULT hr = target->CreateGradientStopCollection(
		value.data(),
		HueGradientPrecision,
		D2D1_GAMMA_1_0,
		D2D1_EXTEND_MODE_CLAMP,
		&gradientStops
	);

	if (FAILED(hr))
	{
		return hr;
	}

	const D2D1_SIZE_F size = target->GetSize();
	hr = target->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(
			D2D1::Point2F(vertical ? 0.0f : size.width, 0.0f),
			D2D1::Point2F(0.0f, vertical ? size.height : 0.0f)
		),
		gradientStops,
		gradient
	);

	gradientStops->Release();

	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}