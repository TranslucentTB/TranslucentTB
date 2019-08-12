#include "pch.h"
#include "arch.h"
#include <shlobj_core.h>
#include <wil/resource.h>

#include "Pages.FramelessPage.h"
#include "Pages.WelcomePage.h"
#if __has_include("Pages.WelcomePage.g.cpp")
#include "Pages.WelcomePage.g.cpp"
#endif

#include "appinfo.hpp"
#include "win32.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	WelcomePage::WelcomePage(hstring configFile) : m_ConfigFile(std::move(configFile))
	{
		InitializeComponent();
		Title(L"Welcome to " APP_NAME L"!");
	}

	void WelcomePage::ForwardActionClick(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::Controls::ItemClickEventArgs &args)
	{
		args.ClickedItem().as<Models::ActionItem>().ForwardClick(sender, args);
	}

	fire_and_forget WelcomePage::RevealConfigFile(const IInspectable &sender, const RoutedEventArgs &args)
	{
		const std::filesystem::path file = static_cast<std::wstring_view>(m_ConfigFile);

		co_await winrt::resume_background();

		winrt::check_hresult(win32::RevealFile(file));
	}

	void WelcomePage::EditConfigFile(const IInspectable &sender, const RoutedEventArgs &args)
	{
		winrt::check_hresult(win32::EditFile(static_cast<std::wstring_view>(m_ConfigFile)));
	}
}
