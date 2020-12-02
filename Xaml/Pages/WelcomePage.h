#pragma once
#include "../factory.h"

#include "FramelessPage.h"
#include "Pages/WelcomePage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage>
	{
		WelcomePage(bool hasPackageIdentity);

		Windows::Foundation::Rect DragRegion() override;

		void OpenLiberapayLink(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void OpenDiscordLink(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void EditConfigFile(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		void AgreeButtonClicked(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void DisagreeButtonClicked(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		event_token LiberapayOpenRequested(const LiberapayOpenDelegate &handler);
		void LiberapayOpenRequested(const event_token &token);

		event_token DiscordJoinRequested(const DiscordJoinDelegate &handler);
		void DiscordJoinRequested(const event_token &token);

		event_token ConfigEditRequested(const ConfigEditDelegate &handler);
		void ConfigEditRequested(const event_token &token);

		event_token LicenseApproved(const LicenseApprovedDelegate &handler);
		void LicenseApproved(const event_token &token);

	private:
		event<LiberapayOpenDelegate> m_LiberapayOpenRequestedHandler;
		event<DiscordJoinDelegate> m_DiscordJoinRequestedHandler;
		event<ConfigEditDelegate> m_ConfigEditRequestedHandler;
		event<LicenseApprovedDelegate> m_LicenseApprovedHandler;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, WelcomePage);
