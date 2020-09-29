#pragma once

#include "Pages/FramelessPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage>
	{
		FramelessPage();

		virtual bool RequestClose();

		void Close();
		event_token Closed(const ClosedDelegate &handler);
		void Closed(const winrt::event_token &token);

		void CloseButtonClicked(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		hstring Title();
		void Title(const hstring &title);
		static Windows::UI::Xaml::DependencyProperty TitleProperty() noexcept;

		Windows::UI::Xaml::UIElement UserContent();
		void UserContent(const Windows::UI::Xaml::UIElement &element);
		static Windows::UI::Xaml::DependencyProperty UserContentProperty() noexcept;

		bool IsClosable();
		void IsClosable(bool closeable);
		static Windows::UI::Xaml::DependencyProperty IsClosableProperty() noexcept;

	private:
		event<ClosedDelegate> m_ClosedHandler;

		static Windows::UI::Xaml::DependencyProperty s_TitleProperty;
		static Windows::UI::Xaml::DependencyProperty s_UserContentProperty;
		static Windows::UI::Xaml::DependencyProperty s_IsClosableProperty;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage, implementation::FramelessPage>
	{
	};
}
