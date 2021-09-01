#include "pch.h"

#include "ThicknessFilterConverter.h"
#if __has_include("Converters/ThicknessFilterConverter.g.cpp")
#include "Converters/ThicknessFilterConverter.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Converters::implementation
{
	wf::IInspectable ThicknessFilterConverter::Convert(const IInspectable &value, const wux::Interop::TypeName &, const IInspectable &, const hstring &)
	{
		auto thickness = unbox_value<wux::Thickness>(value);

		const auto scale = Scale();
		if (!std::isnan(scale))
		{
			thickness.Left *= scale;
			thickness.Top *= scale;
			thickness.Right *= scale;
			thickness.Bottom *= scale;
		}

		return box_value(ApplyFilter(thickness, Filter()));
	}

	wf::IInspectable ThicknessFilterConverter::ConvertBack(const IInspectable &, const wux::Interop::TypeName &, const IInspectable &, const hstring&)
	{
		return wux::DependencyProperty::UnsetValue();
	}

	wux::Thickness ThicknessFilterConverter::ApplyFilter(wux::Thickness thickness, txmp::ThicknessFilterKind filter)
	{
		switch (filter)
		{
		case txmp::ThicknessFilterKind::Left:
			thickness.Left = 0.0;
			break;

		case txmp::ThicknessFilterKind::Top:
			thickness.Top = 0.0;
			break;

		case txmp::ThicknessFilterKind::Right:
			thickness.Right = 0.0;
			break;

		case txmp::ThicknessFilterKind::Bottom:
			thickness.Bottom = 0.0;
			break;

		case txmp::ThicknessFilterKind::LeftRight:
			thickness.Left = 0.0;
			thickness.Right = 0.0;
			break;

		case txmp::ThicknessFilterKind::TopBottom:
			thickness.Top = 0.0;
			thickness.Bottom = 0.0;
			break;
		}

		return thickness;
	}
}
