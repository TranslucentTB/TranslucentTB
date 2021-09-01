#include "pch.h"
#include "colorpickerrendering.hpp"

void FillChannelBitmap(uint8_t *bgraPixelData, int32_t width, int32_t height, double renderScale, wuxc::Orientation orientation, txmp::ColorRepresentation colorRepresentation, txmp::ColorChannel channel, Util::HsvColor baseHsvColor, Util::Color checkerColor, bool isAlphaMaxForced, bool isSaturationValueMaxForced)
{
	// Maximize alpha channel value
	if (isAlphaMaxForced && channel != txmp::ColorChannel::Alpha)
	{
		baseHsvColor.A = 1.0;
	}

	// Convert HSV to RGB once
	const Util::Color baseRgbColor = colorRepresentation == txmp::ColorRepresentation::Rgba
		? Util::Color::FromHSV(baseHsvColor)
		: Util::Color { 0xFF, 0xFF, 0xFF, 0xFF }; // Colors::White

	// Maximize Saturation and Value channels when in HSVA mode
	if (isSaturationValueMaxForced && colorRepresentation == txmp::ColorRepresentation::Hsva && channel != txmp::ColorChannel::Alpha)
	{
		switch (channel)
		{
		case txmp::ColorChannel::Channel1:
			baseHsvColor.S = 1.0;
			baseHsvColor.V = 1.0;
			break;
		case txmp::ColorChannel::Channel2:
			baseHsvColor.V = 1.0;
			break;
		case txmp::ColorChannel::Channel3:
			baseHsvColor.S = 1.0;
			break;
		}
	}

	// Determine the numerical increment of the color steps within the channel
	double channelStep;
	if (colorRepresentation == txmp::ColorRepresentation::Hsva)
	{
		if (channel == txmp::ColorChannel::Channel1)
		{
			channelStep = 360.0;
		}
		else
		{
			channelStep = 1.0;
		}
	}
	else
	{
		channelStep = 255.0;
	}

	// Create the color channel gradient
	if (orientation == wuxc::Orientation::Horizontal)
	{
		channelStep /= width;

		for (int32_t x = 0; x < width; x++)
		{
			const auto rgbColor = GetChannelPixelColor(x * channelStep, channel, colorRepresentation, baseHsvColor, baseRgbColor);

			for (int32_t y = 0; y < height; y++)
			{
				const auto finalColor = !isAlphaMaxForced ? CompositeColors(GetCheckerPixelColor(x, y, checkerColor, renderScale), rgbColor) : rgbColor;

				// 4 bytes per pixel
				const int32_t pixelDataIndex = 4 * (x + (width * y));

				bgraPixelData[pixelDataIndex + 0] = finalColor.B;
				bgraPixelData[pixelDataIndex + 1] = finalColor.G;
				bgraPixelData[pixelDataIndex + 2] = finalColor.R;
				bgraPixelData[pixelDataIndex + 3] = finalColor.A;
			}
		}
	}
	else
	{
		channelStep /= height;

		for (int32_t y = 0; y < height; ++y)
		{
			// The lowest channel value should be at the 'bottom' of the bitmap
			const auto rgbColor = GetChannelPixelColor((height - 1 - y) * channelStep, channel, colorRepresentation, baseHsvColor, baseRgbColor);

			const int32_t pixelRowIndex = width * y;

			for (int32_t x = 0; x < width; ++x)
			{
				const auto finalColor = !isAlphaMaxForced ? CompositeColors(GetCheckerPixelColor(x, y, checkerColor, renderScale), rgbColor) : rgbColor;

				// 4 bytes per pixel
				const int32_t pixelDataIndex = 4 * (x + pixelRowIndex);

				bgraPixelData[pixelDataIndex + 0] = finalColor.B;
				bgraPixelData[pixelDataIndex + 1] = finalColor.G;
				bgraPixelData[pixelDataIndex + 2] = finalColor.R;
				bgraPixelData[pixelDataIndex + 3] = finalColor.A;
			}
		}
	}
}

Util::Color GetChannelPixelColor(double channelValue, txmp::ColorChannel channel, txmp::ColorRepresentation representation, Util::HsvColor baseHsvColor, Util::Color baseRgbColor)
{
	Util::Color newRgbColor = { 0xFF, 0xFF, 0xFF, 0xFF };

	switch (channel)
	{
	case txmp::ColorChannel::Channel1:
		if (representation == txmp::ColorRepresentation::Hsva)
		{
			// Sweep hue
			baseHsvColor.H = std::clamp(channelValue, 0.0, 360.0);
			newRgbColor = Util::Color::FromHSV(baseHsvColor);
		}
		else
		{
			// Sweep red
			baseRgbColor.R = static_cast<uint8_t>(std::clamp(channelValue, 0.0, 255.0));
			newRgbColor = baseRgbColor;
		}

		break;

	case txmp::ColorChannel::Channel2:
		if (representation == txmp::ColorRepresentation::Hsva)
		{
			// Sweep saturation
			baseHsvColor.S = std::clamp(channelValue, 0.0, 1.0);
			newRgbColor = Util::Color::FromHSV(baseHsvColor);
		}
		else
		{
			// Sweep green
			baseRgbColor.G = static_cast<uint8_t>(std::clamp(channelValue, 0.0, 255.0));
			newRgbColor = baseRgbColor;
		}

		break;

	case txmp::ColorChannel::Channel3:
		if (representation == txmp::ColorRepresentation::Hsva)
		{
			// Sweep value
			baseHsvColor.V = std::clamp(channelValue, 0.0, 1.0);
			newRgbColor = Util::Color::FromHSV(baseHsvColor);
		}
		else
		{
			// Sweep blue
			baseRgbColor.B = static_cast<uint8_t>(std::clamp(channelValue, 0.0, 255.0));
			newRgbColor = baseRgbColor;
		}

		break;

	case txmp::ColorChannel::Alpha:
		if (representation == txmp::ColorRepresentation::Hsva)
		{
			// Sweep alpha
			baseHsvColor.A = std::clamp(channelValue, 0.0, 1.0);
			newRgbColor = Util::Color::FromHSV(baseHsvColor);
		}
		else
		{
			// Sweep alpha
			baseRgbColor.A = static_cast<uint8_t>(std::clamp(channelValue, 0.0, 255.0));
			newRgbColor = baseRgbColor;
		}

		break;
	}

	return newRgbColor.Premultiply();
}
