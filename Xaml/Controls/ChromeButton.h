#pragma once

#include "Controls/ChromeButton.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton>
	{
		ChromeButton();

		Windows::UI::Xaml::Controls::IconElement Icon();
		void Icon(Windows::UI::Xaml::Controls::IconElement icon);
		static Windows::UI::Xaml::DependencyProperty IconProperty() noexcept;

		Windows::UI::Xaml::Media::SolidColorBrush HoverForeground();
		void HoverForeground(Windows::UI::Xaml::Media::SolidColorBrush color);
		static Windows::UI::Xaml::DependencyProperty HoverForegroundProperty() noexcept;

		Windows::UI::Xaml::Media::SolidColorBrush HoverBackground();
		void HoverBackground(Windows::UI::Xaml::Media::SolidColorBrush color);
		static Windows::UI::Xaml::DependencyProperty HoverBackgroundProperty() noexcept;

		Windows::UI::Xaml::Media::SolidColorBrush PressedForeground();
		void PressedForeground(Windows::UI::Xaml::Media::SolidColorBrush color);
		static Windows::UI::Xaml::DependencyProperty PressedForegroundProperty() noexcept;

		Windows::UI::Xaml::Media::SolidColorBrush PressedBackground();
		void PressedBackground(Windows::UI::Xaml::Media::SolidColorBrush color);
		static Windows::UI::Xaml::DependencyProperty PressedBackgroundProperty() noexcept;

	private:
		static Windows::UI::Xaml::DependencyProperty s_IconProperty;
		static Windows::UI::Xaml::DependencyProperty s_HoverForegroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_HoverBackgroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_PressedForegroundProperty;
		static Windows::UI::Xaml::DependencyProperty s_PressedBackgroundProperty;
	};
}

namespace winrt::TranslucentTB::Xaml::Controls::factory_implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton, implementation::ChromeButton>
	{
	};
}
