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
		const wil::unique_cotaskmem_ptr<ITEMIDLIST_ABSOLUTE> list(ILCreateFromPath(m_ConfigFile.c_str()));

		co_await winrt::resume_background();

		winrt::check_hresult(SHOpenFolderAndSelectItems(list.get(), 0, nullptr, 0));
	}
}
