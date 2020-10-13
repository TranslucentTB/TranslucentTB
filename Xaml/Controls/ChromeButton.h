#pragma once

#include "Controls/ChromeButton.g.h"

// FIXME: ugly workaround for https://github.com/microsoft/microsoft-ui-xaml/issues/2429
using namespace winrt::TranslucentTB::Xaml::implementation;

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton>
	{
		ChromeButton();

		Windows::UI::Xaml::Controls::IconElement Icon();
		void Icon(Windows::UI::Xaml::Controls::IconElement icon);
		static Windows::UI::Xaml::DependencyProperty IconProperty() noexcept;

		Windows::UI::Color NormalForeground();
		void NormalForeground(const Windows::UI::Color &color);
		static Windows::UI::Xaml::DependencyProperty NormalForegroundProperty() noexcept;

		Windows::UI::Color NormalBackground();
		void NormalBackground(const Windows::UI::Color &color);
		static Windows::UI::Xaml::DependencyProperty NormalBackgroundProperty() noexcept;

		Windows::UI::Color HoverForeground();
		void HoverForeground(const Windows::UI::Color &color);
		static Windows::UI::Xaml::DependencyProperty HoverForegroundProperty() noexcept;

		Windows::UI::Color HoverBackground();
		void HoverBackground(const Windows::UI::Color &color);
		static Windows::UI::Xaml::DependencyProperty HoverBackgroundProperty() noexcept;

		Windows::UI::Color PressedForeground();
		void PressedForeground(const Windows::UI::Color &color);
		static Windows::UI::Xaml::DependencyProperty PressedForegroundProperty() noexcept;

		Windows::UI::Color PressedBackground();
		void PressedBackground(const Windows::UI::Color &color);
		static Windows::UI::Xaml::DependencyProperty PressedBackgroundProperty() noexcept;

		bool IsTogglable();
		void IsTogglable(bool value);
		static Windows::UI::Xaml::DependencyProperty IsTogglableProperty() noexcept;

		void OnToggle();

	private:
		static Windows::UI::Xaml::DependencyProperty s_IconProperty;
		static Windows::UI::Xaml::DependencyProperty s_NormalForegroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_NormalBackgroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_HoverForegroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_HoverBackgroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_PressedForegroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_PressedBackgroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_IsTogglableProperty;
	};
}

namespace winrt::TranslucentTB::Xaml::Controls::factory_implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton, implementation::ChromeButton>
	{
	};
}
