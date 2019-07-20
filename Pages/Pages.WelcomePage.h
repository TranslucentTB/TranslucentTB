#pragma once

#include "Pages.WelcomePage.g.h"

namespace winrt::TranslucentTB::Pages::implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage>
	{
		WelcomePage(hstring configFile);

		hstring ConfigFile();

		void OpenConfigFile(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

	private:
		hstring m_ConfigFile;
	};
}

namespace winrt::TranslucentTB::Pages::factory_implementation
{
	struct WelcomePage : WelcomePageT<WelcomePage, implementation::WelcomePage>
	{
	};
}
