#pragma once
#include <cstdint>
#include <vector>

#include "util/color.hpp"

constexpr Util::Color GetCheckerPixelColor(int32_t x, int32_t y, Util::Color checkerColor, double renderScale, bool invert = false) noexcept
{
	constexpr int32_t CHECKER_SIZE_PRE_SCALE = 4;
	const int32_t checkerSize = static_cast<int32_t>(CHECKER_SIZE_PRE_SCALE * renderScale);

	// We want the checkered pattern to alternate both vertically and horizontally.
	// In order to achieve that, we'll toggle visibility of the current pixel on or off
	// depending on both its x- and its y-position.  If x == CheckerSize, we'll turn visibility off,
	// but then if y == CheckerSize, we'll turn it back on.
	// The below is a shorthand for the above intent.
	bool pixelShouldBeBlank = (x / checkerSize + y / checkerSize) % 2 == 0 ? 255 : 0;

	if (invert)
	{
		pixelShouldBeBlank = !pixelShouldBeBlank;
	}

	if (pixelShouldBeBlank)
	{
		return { };
	}
	else
	{
		return checkerColor.Premultiply();
	}
}

constexpr Util::Color CompositeColors(Util::Color bottom, Util::Color top) noexcept
{
	/* The following algorithm is used to blend the two bitmaps creating the final composite.
	 * In this formula, pixel data is normalized 0..1, actual pixel data is in the range 0..255.
	 * The color channel gradient should apply OVER the checkered background.
	 *
	 * R =  R0 * A0 * (1 - A1) + R1 * A1  =  RA0 * (1 - A1) + RA1
	 * G =  G0 * A0 * (1 - A1) + G1 * A1  =  GA0 * (1 - A1) + GA1
	 * B =  B0 * A0 * (1 - A1) + B1 * A1  =  BA0 * (1 - A1) + BA1
	 * A =  A0 * (1 - A1) + A1            =  A0 * (1 - A1) + A1
	 *
	 * Considering only the red channel, some algebraic transformation is applied to
	 * make the math quicker to solve.
	 *
	 * => ((RA0 / 255.0) * (1.0 - A1 / 255.0) + (RA1 / 255.0)) * 255.0
	 * => ((RA0 * 255) - (RA0 * A1) + (RA1 * 255)) / 255
	 */

	return {
		static_cast<uint8_t>(((bottom.R * 255) - (bottom.R * top.A) + (top.R * 255)) / 255),
		static_cast<uint8_t>(((bottom.G * 255) - (bottom.G * top.A) + (top.G * 255)) / 255),
		static_cast<uint8_t>(((bottom.B * 255) - (bottom.B * top.A) + (top.B * 255)) / 255),
		static_cast<uint8_t>(((bottom.A * 255) - (bottom.A * top.A) + (top.A * 255)) / 255)
	};
}

constexpr void FillCheckeredBitmap(uint8_t *bgraPixelData, int32_t width, int32_t height, double renderScale, Util::Color checkerColor, bool invert) noexcept
{
	for (int y = 0; y < height; y++)
	{
		const int32_t pixelRowIndex = width * y;

		for (int x = 0; x < width; x++)
		{
			const auto rgbColor = GetCheckerPixelColor(x, y, checkerColor, renderScale, invert);

			const int32_t pixelDataIndex = 4 * (x + pixelRowIndex);

			bgraPixelData[pixelDataIndex + 0] = rgbColor.B;
			bgraPixelData[pixelDataIndex + 1] = rgbColor.G;
			bgraPixelData[pixelDataIndex + 2] = rgbColor.R;
			bgraPixelData[pixelDataIndex + 3] = rgbColor.A;
		}
	}
}

void FillChannelBitmap(uint8_t *bgraPixelData, int32_t width, int32_t height, double renderScale, wuxc::Orientation orientation, txmp::ColorRepresentation colorRepresentation, txmp::ColorChannel channel, Util::HsvColor baseHsvColor, Util::Color checkerColor, bool isAlphaMaxForced, bool isSaturationValueMaxForced);
Util::Color GetChannelPixelColor(double channelValue, txmp::ColorChannel channel, txmp::ColorRepresentation representation, Util::HsvColor baseHsvColor, Util::Color baseRgbColor);
