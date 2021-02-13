#pragma once
#include "../event.h"
#include "../factory.h"
#include "winrt.hpp"

#include "FramelessPage.h"
#include "Pages/WelcomePage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage>
	{
		WelcomePage(bool hasPackageIdentity);

		wf::Rect DragRegion() override;

		void OpenLiberapayLink(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void OpenDiscordLink(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void EditConfigFile(const IInspectable &sender, const wux::RoutedEventArgs &args);

		void AgreeButtonClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void DisagreeButtonClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);

		DECL_EVENT_FUNCS(LiberapayOpenDelegate, LiberapayOpenRequested, m_LiberapayOpenRequestedHandler);
		DECL_EVENT_FUNCS(DiscordJoinDelegate, DiscordJoinRequested, m_DiscordJoinRequestedHandler);
		DECL_EVENT_FUNCS(ConfigEditDelegate, ConfigEditRequested, m_ConfigEditRequestedHandler);
		DECL_EVENT_FUNCS(LicenseApprovedDelegate, LicenseApproved, m_LicenseApprovedHandler);

	private:
		event<LiberapayOpenDelegate> m_LiberapayOpenRequestedHandler;
		event<DiscordJoinDelegate> m_DiscordJoinRequestedHandler;
		event<ConfigEditDelegate> m_ConfigEditRequestedHandler;
		event<LicenseApprovedDelegate> m_LicenseApprovedHandler;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, WelcomePage);
