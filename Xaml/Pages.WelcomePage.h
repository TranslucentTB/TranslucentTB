#pragma once

#include "Pages.WelcomePage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage>
	{
		WelcomePage();

		void ForwardActionClick(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::Controls::ItemClickEventArgs &args);
		void OpenLiberapayLink(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void OpenDiscordLink(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void EditConfigFile(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		event_token DiscordJoinRequested(const DiscordJoinDelegate &handler);
		void DiscordJoinRequested(const winrt::event_token& token) noexcept;

		event_token ConfigEditRequested(const ConfigEditDelegate &handler);
		void ConfigEditRequested(const winrt::event_token &token) noexcept;

	private:
		event<DiscordJoinDelegate> m_DiscordJoinRequestedHandler;
		event<ConfigEditDelegate> m_ConfigEditRequestedHandler;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage, implementation::WelcomePage>
	{
	};
}
