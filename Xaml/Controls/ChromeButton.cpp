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

	DependencyProperty ChromeButton::s_HoverForegroundProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(HoverForeground),
			xaml_typename<Media::SolidColorBrush>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty ChromeButton::s_HoverBackgroundProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(HoverBackground),
			xaml_typename<Media::SolidColorBrush>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty ChromeButton::s_PressedForegroundProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(PressedForeground),
			xaml_typename<Media::SolidColorBrush>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty ChromeButton::s_PressedBackgroundProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(PressedBackground),
			xaml_typename<Media::SolidColorBrush>(),
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

	Media::SolidColorBrush ChromeButton::HoverForeground()
	{
		return GetValue(s_HoverForegroundProperty).as<Media::SolidColorBrush>();
	}

	void ChromeButton::HoverForeground(Media::SolidColorBrush color)
	{
		SetValue(s_HoverForegroundProperty, color);
	}

	Windows::UI::Xaml::DependencyProperty ChromeButton::HoverForegroundProperty() noexcept
	{
		return s_HoverForegroundProperty;
	}

	Media::SolidColorBrush ChromeButton::HoverBackground()
	{
		return GetValue(s_HoverBackgroundProperty).as<Media::SolidColorBrush>();
	}

	void ChromeButton::HoverBackground(Media::SolidColorBrush color)
	{
		SetValue(s_HoverBackgroundProperty, color);
	}

	Windows::UI::Xaml::DependencyProperty ChromeButton::HoverBackgroundProperty() noexcept
	{
		return s_HoverBackgroundProperty;
	}

	Media::SolidColorBrush ChromeButton::PressedForeground()
	{
		return GetValue(s_PressedForegroundProperty).as<Media::SolidColorBrush>();
	}

	void ChromeButton::PressedForeground(Media::SolidColorBrush color)
	{
		SetValue(s_PressedForegroundProperty, color);
	}

	Windows::UI::Xaml::DependencyProperty ChromeButton::PressedForegroundProperty() noexcept
	{
		return s_PressedForegroundProperty;
	}

	Media::SolidColorBrush ChromeButton::PressedBackground()
	{
		return GetValue(s_PressedBackgroundProperty).as<Media::SolidColorBrush>();
	}

	void ChromeButton::PressedBackground(Media::SolidColorBrush color)
	{
		SetValue(s_PressedBackgroundProperty, color);
	}

	Windows::UI::Xaml::DependencyProperty ChromeButton::PressedBackgroundProperty() noexcept
	{
		return s_PressedBackgroundProperty;
	}
}
