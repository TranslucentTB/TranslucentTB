#include "pch.h"

#include "Converters/ModifyOpacitySolidColorBrushToColorValueConverter.h"
#if __has_include("Converters/ModifyOpacitySolidColorBrushToColorValueConverter.g.cpp")
#include "Converters/ModifyOpacitySolidColorBrushToColorValueConverter.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	Windows::Foundation::IInspectable ModifyOpacitySolidColorBrushToColorValueConverter::Convert(const Windows::Foundation::IInspectable &value, const Windows::UI::Xaml::Interop::TypeName &, const Windows::Foundation::IInspectable &parameter, const hstring &)
	{
		auto color = value.as<Media::SolidColorBrush>().Color();
		color.A = unbox_value_or<uint8_t>(parameter, 0);
		return box_value(color);
	}

	Windows::Foundation::IInspectable ModifyOpacitySolidColorBrushToColorValueConverter::ConvertBack(const Windows::Foundation::IInspectable &, const Windows::UI::Xaml::Interop::TypeName &, const Windows::Foundation::IInspectable &, const hstring &)
	{
		throw hresult_not_implemented(L"Cannot convert back from this converter");
	}
}
