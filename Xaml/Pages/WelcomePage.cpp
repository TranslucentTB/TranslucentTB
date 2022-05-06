#include "pch.h"

#include "WelcomePage.h"
#if __has_include("Pages/WelcomePage.g.cpp")
#include "Pages/WelcomePage.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	wf::Rect WelcomePage::DragRegion()
	{
		const auto titleRegion = TitleRegion();
		return {
			0.0f,
			0.0f,
			static_cast<float>(titleRegion.ActualWidth()),
			static_cast<float>(titleRegion.ActualHeight())
		};
	}

	void WelcomePage::OpenLiberapayLink(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_LiberapayOpenRequestedHandler();
	}

	void WelcomePage::OpenDiscordLink(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_DiscordJoinRequestedHandler();
	}

	void WelcomePage::EditConfigFile(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_ConfigEditRequestedHandler();
	}

	void WelcomePage::AgreeButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_LicenseApprovedHandler();
		Close();
	}

	void WelcomePage::DisagreeButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		Close();
	}
}
