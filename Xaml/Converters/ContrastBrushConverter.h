#pragma once
#include "../factory.h"
#include "winrt.hpp"

#include "Converters/ContrastBrushConverter.g.h"

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	struct ContrastBrushConverter : ContrastBrushConverterT<ContrastBrushConverter>
	{
		ContrastBrushConverter() = default;

		uint8_t AlphaThreshold() noexcept;
		void AlphaThreshold(uint8_t value) noexcept;

		IInspectable Convert(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);
		IInspectable ConvertBack(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);

	private:
		uint8_t m_AlphaThreshold = 128;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Converters, ContrastBrushConverter);
