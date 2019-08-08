#include "pch.h"

#include "Pages.FramelessPage.h"
#include "Pages.WelcomePage.h"
#if __has_include("Pages.WelcomePage.g.cpp")
#include "Pages.WelcomePage.g.cpp"
#endif

#include "appinfo.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	WelcomePage::WelcomePage(hstring configFile) : m_ConfigFile(std::move(configFile))
	{
		InitializeComponent();
		Title(L"Welcome to " APP_NAME L"!");
	}

	hstring WelcomePage::ConfigFile()
	{
		return m_ConfigFile;
	}

	void WelcomePage::OpenConfigFile(const IInspectable &sender, const RoutedEventArgs &args)
	{
		// TODO: make work
		OutputDebugString(m_ConfigFile.c_str());
	}
}
