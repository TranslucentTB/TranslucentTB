#pragma once
#include "../factory.h"
#include "winrt.hpp"

#include "Converters/ColorToColorShadeConverter.g.h"

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	struct ColorToColorShadeConverter : ColorToColorShadeConverterT<ColorToColorShadeConverter>
	{
		ColorToColorShadeConverter() = default;

		IInspectable Convert(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);
		IInspectable ConvertBack(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);

	private:
		static Windows::UI::Color GetShade(Windows::UI::Color col, int shade);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Converters, ColorToColorShadeConverter);
