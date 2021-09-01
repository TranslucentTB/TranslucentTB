#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Converters/ThicknessFilterConverter.g.h"

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	struct ThicknessFilterConverter : ThicknessFilterConverterT<ThicknessFilterConverter>
	{
		ThicknessFilterConverter() = default;

		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(txmp::ThicknessFilterKind, Filter, box_value(txmp::ThicknessFilterKind::None));
		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(double, Scale, box_value(1.0));

		IInspectable Convert(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);
		IInspectable ConvertBack(const IInspectable &value, const wux::Interop::TypeName &targetType, const IInspectable &parameter, const hstring &language);

	private:
		static wux::Thickness ApplyFilter(wux::Thickness thickness, txmp::ThicknessFilterKind filter);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Converters, ThicknessFilterConverter);
