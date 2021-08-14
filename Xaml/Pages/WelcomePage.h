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

		DECL_EVENT(LiberapayOpenDelegate, LiberapayOpenRequested, m_LiberapayOpenRequestedHandler);
		DECL_EVENT(DiscordJoinDelegate, DiscordJoinRequested, m_DiscordJoinRequestedHandler);
		DECL_EVENT(ConfigEditDelegate, ConfigEditRequested, m_ConfigEditRequestedHandler);
		DECL_EVENT(LicenseApprovedDelegate, LicenseApproved, m_LicenseApprovedHandler);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, WelcomePage);
