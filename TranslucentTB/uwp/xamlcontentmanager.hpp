#pragma once
#include "arch.h"
#include <utility>
#include <vector>
#include <windef.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/TranslucentTB.Xaml.h>
#include <winrt/Windows.System.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

#include "xamlpagehost.hpp"

class XamlContentManager {
	winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_Manager;
	winrt::TranslucentTB::Xaml::App m_App;
	std::vector<winrt::weak_ref<IDesktopWindowXamlSourceNative2>> m_Sources;

	HINSTANCE m_hInst;
	winrt::Windows::System::DispatcherQueue m_Dispatcher;

	void InitializeXamlHosting();

public:
	XamlContentManager(const XamlContentManager &) = delete;
	XamlContentManager &operator =(const XamlContentManager &) = delete;

	inline XamlContentManager(winrt::Windows::System::DispatcherQueue dispatcher, HINSTANCE hInst) noexcept :
		m_Manager(nullptr),
		m_App(nullptr),
		m_hInst(hInst),
		m_Dispatcher(std::move(dispatcher))
	{ }

	bool PreTranslateMessage(const MSG &msg);

	template<typename T, typename... Args>
	T CreateXamlWindow(xaml_startup_position pos, Args&&... args)
	{
		if (!m_Manager || !m_App)
		{
			InitializeXamlHosting();
		}

		// intentional leak, host manages its own lifetime
		const auto host = new XamlPageHost<T>(m_hInst, pos, m_Dispatcher, std::forward<Args>(args)...);
		return host->content();
	}
};
