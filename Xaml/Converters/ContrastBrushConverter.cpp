#include "pch.h"

#include "ContrastBrushConverter.h"
#if __has_include("Converters/ContrastBrushConverter.g.cpp")
#include "Converters/ContrastBrushConverter.g.cpp"
#endif

#include "util/color.hpp"

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	uint8_t ContrastBrushConverter::AlphaThreshold() noexcept
	{
		return m_AlphaThreshold;
	}

	void ContrastBrushConverter::AlphaThreshold(uint8_t value) noexcept
	{
		m_AlphaThreshold = value;
	}

	wf::IInspectable ContrastBrushConverter::Convert(const IInspectable &value, const wux::Interop::TypeName &, const IInspectable &parameter, const hstring &)
	{
		Windows::UI::Color comparisonColor;
		std::optional<Windows::UI::Color> defaultColor;

		if (const auto valueColor = value.try_as<Windows::UI::Color>())
		{
			comparisonColor = *valueColor;
		}
		else if (const auto valueBrush = value.try_as<wux::Media::SolidColorBrush>())
		{
			comparisonColor = valueBrush.Color();
		}
		else
		{
			return wux::DependencyProperty::UnsetValue();
		}

		if (const auto parameterColor = parameter.try_as<Windows::UI::Color>())
		{
			defaultColor = *parameterColor;
		}
		else if (const auto parameterBrush = parameter.try_as<wux::Media::SolidColorBrush>())
		{
			defaultColor = parameterBrush.Color();
		}

		Windows::UI::Color resultColor;
		if (comparisonColor.A < m_AlphaThreshold && defaultColor)
		{
			resultColor = *defaultColor;
		}
		else if (Util::Color(comparisonColor).IsDarkColor())
		{
			// White
			resultColor = { 0xFF, 0xFF, 0xFF, 0xFF };
		}
		else
		{
			// Black
			resultColor = { 0xFF, 0x00, 0x00, 0x00 };
		}

		return wux::Media::SolidColorBrush(resultColor);
	}

	wf::IInspectable ContrastBrushConverter::ConvertBack(const IInspectable &, const wux::Interop::TypeName &, const IInspectable &, const hstring &)
	{
		return wux::DependencyProperty::UnsetValue();
	}
}
