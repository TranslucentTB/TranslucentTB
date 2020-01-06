#pragma once

#include "Pages.FramelessPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage>
	{
		FramelessPage();

		hstring Title();
		void Title(hstring title);

		Windows::UI::Xaml::UIElement UserContent();
		void UserContent(Windows::UI::Xaml::UIElement element);

		static Windows::UI::Xaml::DependencyProperty TitleProperty() noexcept;
		static Windows::UI::Xaml::DependencyProperty UserContentProperty() noexcept;

	private:
		static Windows::UI::Xaml::DependencyProperty s_TitleProperty;
		static Windows::UI::Xaml::DependencyProperty s_UserContentProperty;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage, implementation::FramelessPage>
	{
	};
}
