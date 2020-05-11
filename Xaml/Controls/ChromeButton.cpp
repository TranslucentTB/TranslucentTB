#include "pch.h"

#include "util/strings.hpp"

#include "ChromeButton.h"
#if __has_include("Controls/ChromeButton.g.cpp")
#include "Controls/ChromeButton.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	DependencyProperty ChromeButton::s_IconProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(Icon),
			xaml_typename<Windows::UI::Xaml::Controls::IconElement>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty ChromeButton::s_HoverPressedForegroundProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(HoverPressedForeground),
			xaml_typename<Media::Brush>(),
			xaml_typename<class_type>(),
			nullptr
	);

	ChromeButton::ChromeButton()
	{
		InitializeComponent();
	}

	Windows::UI::Xaml::Controls::IconElement ChromeButton::Icon()
	{
		return GetValue(s_IconProperty).as<Windows::UI::Xaml::Controls::IconElement>();
	}

	void ChromeButton::Icon(Windows::UI::Xaml::Controls::IconElement icon)
	{
		SetValue(s_IconProperty, icon);
	}

	Windows::UI::Xaml::DependencyProperty ChromeButton::IconProperty() noexcept
	{
		return s_IconProperty;
	}

	Media::Brush ChromeButton::HoverPressedForeground()
	{
		return GetValue(s_HoverPressedForegroundProperty).as<Media::Brush>();
	}

	void ChromeButton::HoverPressedForeground(Media::Brush brush)
	{
		SetValue(s_HoverPressedForegroundProperty, brush);
	}

	Windows::UI::Xaml::DependencyProperty ChromeButton::HoverPressedForegroundProperty() noexcept
	{
		return s_HoverPressedForegroundProperty;
	}
}
