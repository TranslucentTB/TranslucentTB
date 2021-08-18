#include "pch.h"

#include "ColorToDisplayNameConverter.h"
#if __has_include("Converters/ColorToDisplayNameConverter.g.cpp")
#include "Converters/ColorToDisplayNameConverter.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	wf::IInspectable ColorToDisplayNameConverter::Convert(const IInspectable &value, const wux::Interop::TypeName&, const IInspectable &, const hstring &)
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

		return box_value(Windows::UI::ColorHelper::ToDisplayName(rgbColor));
	}

	wf::IInspectable ColorToDisplayNameConverter::ConvertBack(const IInspectable &, const wux::Interop::TypeName &, const IInspectable &, const hstring &)
	{
		return wux::DependencyProperty::UnsetValue();
	}
}
