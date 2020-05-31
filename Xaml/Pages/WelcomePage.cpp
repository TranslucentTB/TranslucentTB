#include "pch.h"

#include "FramelessPage.h"
#include "WelcomePage.h"
#if __has_include("Pages/WelcomePage.g.cpp")
#include "Pages/WelcomePage.g.cpp"
#endif

#include "appinfo.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	WelcomePage::WelcomePage(bool hasPackageIdentity)
	{
		InitializeComponent();
		Title(L"Welcome to " APP_NAME L"!");

		if (!hasPackageIdentity)
		{
			StartupCheckbox().Visibility(Visibility::Collapsed);
		}
	}

	void WelcomePage::OpenLiberapayLink(const IInspectable &, const RoutedEventArgs &)
	{
		m_LiberapayOpenRequestedHandler();
	}

	void WelcomePage::OpenDiscordLink(const IInspectable &, const RoutedEventArgs &)
	{
		m_DiscordJoinRequestedHandler();
	}

	void WelcomePage::EditConfigFile(const IInspectable &, const RoutedEventArgs &)
	{
		m_ConfigEditRequestedHandler();
	}

	void WelcomePage::AgreeButtonClicked(const IInspectable &, const RoutedEventArgs &)
	{
		m_LicenseApprovedHandler(*this, StartupCheckbox().IsChecked().Value());
		Close();
	}

	void WelcomePage::DisagreeButtonClicked(const IInspectable &, const RoutedEventArgs &)
	{
		Close();
	}

	event_token WelcomePage::LiberapayOpenRequested(const LiberapayOpenDelegate &handler)
	{
		return m_LiberapayOpenRequestedHandler.add(handler);
	}

	void WelcomePage::LiberapayOpenRequested(const winrt::event_token &token)
	{
		m_LiberapayOpenRequestedHandler.remove(token);
	}

	event_token WelcomePage::DiscordJoinRequested(const DiscordJoinDelegate &handler)
	{
		return m_DiscordJoinRequestedHandler.add(handler);
	}

	void WelcomePage::DiscordJoinRequested(const winrt::event_token &token)
	{
		m_DiscordJoinRequestedHandler.remove(token);
	}

	event_token WelcomePage::ConfigEditRequested(const ConfigEditDelegate &handler)
	{
		return m_ConfigEditRequestedHandler.add(handler);
	}

	void WelcomePage::ConfigEditRequested(const winrt::event_token &token)
	{
		m_ConfigEditRequestedHandler.remove(token);
	}

	event_token WelcomePage::LicenseApproved(const Windows::Foundation::EventHandler<bool> &handler)
	{
		return m_LicenseApprovedHandler.add(handler);
	}

	void WelcomePage::LicenseApproved(const winrt::event_token &token)
	{
		m_LicenseApprovedHandler.remove(token);
	}
}
