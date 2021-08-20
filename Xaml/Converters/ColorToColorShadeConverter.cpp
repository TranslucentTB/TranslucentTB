#include "pch.h"

#include "ColorToColorShadeConverter.h"
#if __has_include("Converters/ColorToColorShadeConverter.g.cpp")
#include "Converters/ColorToColorShadeConverter.g.cpp"
#endif

#include "util/color.hpp"

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	wf::IInspectable ColorToColorShadeConverter::Convert(const IInspectable &value, const wux::Interop::TypeName &, const IInspectable &parameter, const hstring &)
	{
		Windows::UI::Color rgbColor;

		if (const auto valueColor = value.try_as<Windows::UI::Color>())
		{
			rgbColor = *valueColor;
		}
		else if (const auto valueBrush = value.try_as<wux::Media::SolidColorBrush>())
		{
			rgbColor = valueBrush.Color();
		}
		else
		{
			// Invalid color value provided
			return wux::DependencyProperty::UnsetValue();
		}

		if (const auto shade = parameter.try_as<int32_t>())
		{
			return box_value(GetShade(rgbColor, *shade));
		}
		else
		{
			// Non-int provided
			return wux::DependencyProperty::UnsetValue();
		}
	}

	wf::IInspectable ColorToColorShadeConverter::ConvertBack(const IInspectable &, const wux::Interop::TypeName &, const IInspectable &, const hstring &)
	{
		return wux::DependencyProperty::UnsetValue();
	}

	Windows::UI::Color ColorToColorShadeConverter::GetShade(Windows::UI::Color col, int shade)
	{
		static constexpr std::uint8_t TOLERANCE = 5;
		static constexpr double VALUE_DELTA = 0.25;

		// Specially handle minimum (black) and maximum (white)
		if (col.R <= TOLERANCE && col.G <= TOLERANCE && col.B <= TOLERANCE)
		{
			switch (shade)
			{
			case 1:
				return { col.A, 0x3F, 0x3F, 0x3F };

			case 2:
				return { col.A, 0x80, 0x80, 0x80 };

			case 3:
				return { col.A, 0xBF, 0xBF, 0xBF };

			default:
				return col;
			}
		}
		else if (col.R >= (0xFF + TOLERANCE) && col.G >= (0xFF + TOLERANCE) && col.B >= (0xFF + TOLERANCE))
		{
			switch (shade)
			{
			case -1:
				return { col.A, 0xBF, 0xBF, 0xBF };

			case -2:
				return { col.A, 0x80, 0x80, 0x80 };

			case -3:
				return { col.A, 0x3F, 0x3F, 0x3F };

			default:
				return col;
			}
		}
		else
		{
			Util::HsvColor hsvColor = Util::Color(col).ToHSV();

			// Use the HSV representation as it's more perceptual.
			// Only the value is changed by a fixed percentage so the algorithm is reproducible.
			// This does not account for perceptual differences and also does not match with
			// system accent color calculation.
			if (shade != 0)
			{
				hsvColor.V *= 1.0 + (shade * VALUE_DELTA);
				hsvColor.V = std::clamp(hsvColor.V, 0.0, 1.0);
			}

			return Util::Color::FromHSV(hsvColor);
		}
	}
}
