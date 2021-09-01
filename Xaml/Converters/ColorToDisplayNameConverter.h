#pragma once
#include "../factory.h"
#include "winrt.hpp"

#include "Converters/ColorToDisplayNameConverter.g.h"

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	struct ColorToDisplayNameConverter : ColorToDisplayNameConverterT<ColorToDisplayNameConverter>
	{
		ColorToDisplayNameConverter() = default;

		IInspectable Convert(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);
		IInspectable ConvertBack(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Converters, ColorToDisplayNameConverter);
