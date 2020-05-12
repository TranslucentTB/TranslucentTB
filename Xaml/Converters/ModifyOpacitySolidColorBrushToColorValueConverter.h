#pragma once

#include "Converters/ModifyOpacitySolidColorBrushToColorValueConverter.g.h"

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	struct ModifyOpacitySolidColorBrushToColorValueConverter : ModifyOpacitySolidColorBrushToColorValueConverterT<ModifyOpacitySolidColorBrushToColorValueConverter>
	{
		ModifyOpacitySolidColorBrushToColorValueConverter() = default;

		Windows::Foundation::IInspectable Convert(const Windows::Foundation::IInspectable &value, const Windows::UI::Xaml::Interop::TypeName &targetType, const Windows::Foundation::IInspectable &parameter, const hstring &language);
		Windows::Foundation::IInspectable ConvertBack(const Windows::Foundation::IInspectable &value, const Windows::UI::Xaml::Interop::TypeName &targetType, const Windows::Foundation::IInspectable &parameter, const hstring &language);
	};
}

namespace winrt::TranslucentTB::Xaml::Converters::factory_implementation
{
	struct ModifyOpacitySolidColorBrushToColorValueConverter : ModifyOpacitySolidColorBrushToColorValueConverterT<ModifyOpacitySolidColorBrushToColorValueConverter, implementation::ModifyOpacitySolidColorBrushToColorValueConverter>
	{
	};
}
