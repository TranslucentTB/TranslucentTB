#include "pch.h"

#include "WelcomePage.h"
#if __has_include("Pages/WelcomePage.g.cpp")
#include "Pages/WelcomePage.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	WelcomePage::WelcomePage(bool hasPackageIdentity)
	{
		InitializeComponent();

		if (!hasPackageIdentity)
		{
			StartupCheckbox().Visibility(wux::Visibility::Collapsed);
		}
	}

	wf::Rect WelcomePage::DragRegion()
	{
		return {
			0.0f,
			0.0f,
			static_cast<float>(TitleRegion().ActualWidth()),
			static_cast<float>(TitleRegion().ActualHeight())
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
		m_LicenseApprovedHandler(StartupCheckbox().IsChecked().Value());
		Close();
	}

	void WelcomePage::DisagreeButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		Close();
	}
}
