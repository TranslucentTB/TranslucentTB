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

		Windows::UI::Xaml::Media::Brush HoverPressedForeground();
		void HoverPressedForeground(Windows::UI::Xaml::Media::Brush brush);
		static Windows::UI::Xaml::DependencyProperty HoverPressedForegroundProperty() noexcept;

	private:
		static Windows::UI::Xaml::DependencyProperty s_IconProperty;
		static Windows::UI::Xaml::DependencyProperty s_HoverPressedForegroundProperty;
	};
}

namespace winrt::TranslucentTB::Xaml::Controls::factory_implementation
{
	struct ChromeButton : ChromeButtonT<ChromeButton, implementation::ChromeButton>
	{
	};
}
