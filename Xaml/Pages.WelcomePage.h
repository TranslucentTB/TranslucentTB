#pragma once

#include "Pages.WelcomePage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage>
	{
		WelcomePage(bool hasPackageIdentity);

		void ForwardActionClick(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::Controls::ItemClickEventArgs &args);
		void OpenLiberapayLink(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void OpenDiscordLink(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void EditConfigFile(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		void AgreeButtonClicked(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void DisagreeButtonClicked(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		event_token LiberapayOpenRequested(const LiberapayOpenDelegate &handler);
		void LiberapayOpenRequested(const winrt::event_token &token);

		event_token DiscordJoinRequested(const DiscordJoinDelegate &handler);
		void DiscordJoinRequested(const winrt::event_token &token);

		event_token ConfigEditRequested(const ConfigEditDelegate &handler);
		void ConfigEditRequested(const winrt::event_token &token);

		event_token LicenseApproved(const Windows::Foundation::EventHandler<bool> &handler);
		void LicenseApproved(const winrt::event_token &token);

	private:
		event<LiberapayOpenDelegate> m_LiberapayOpenRequestedHandler;
		event<DiscordJoinDelegate> m_DiscordJoinRequestedHandler;
		event<ConfigEditDelegate> m_ConfigEditRequestedHandler;
		event<Windows::Foundation::EventHandler<bool>> m_LicenseApprovedHandler;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage, implementation::WelcomePage>
	{
	};
}
