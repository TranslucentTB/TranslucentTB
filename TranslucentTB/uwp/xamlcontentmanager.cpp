#include "xamlcontentmanager.hpp"
#include <CoreWindow.h>
#include <winrt/Windows.UI.Core.h>

#include "../../ProgramLog/error/win32.hpp"
#include "window.hpp"

void XamlContentManager::InitializeXamlHosting()
{
	// Order is important.
	// We get catastrophic failures if we initialize the WindowsXamlManager before our app. 
	try
	{
		m_App = { };
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to load XAML resources");

	try
	{
		m_Manager = winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to initialize WindowsXamlManager");

	Window coreWin;
	try
	{
		winrt::check_hresult(winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().as<ICoreWindowInterop>()->get_WindowHandle(coreWin.put()));
	}
	HresultErrorCatch(spdlog::level::warn, L"Failed to get core window handle");

	if (coreWin)
	{
		if (!coreWin.show(SW_HIDE))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to hide core window");
		}
	}
}

bool XamlContentManager::PreTranslateMessage(const MSG &msg)
{
	// prevent XAML islands from capturing ALT-F4 because of
	// https://github.com/microsoft/microsoft-ui-xaml/issues/2408
	if (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_F4) [[unlikely]]
	{
		Window(msg.hwnd).ancestor(GA_ROOT).send_message(msg.message, msg.wParam, msg.lParam);
		return true;
	}

	for (auto it = m_Sources.begin(); it != m_Sources.end();)
	{
		if (const auto source = it->get())
		{
			BOOL result;
			const HRESULT hr = source->PreTranslateMessage(&msg, &result);
			if (SUCCEEDED(hr))
			{
				if (result)
				{
					return result;
				}
			}
			else
			{
				HresultHandle(hr, spdlog::level::warn, L"Failed to pre-translate message");
			}

			++it;
		}
		else
		{
			it = m_Sources.erase(it);
		}
	}

	return false;
}
