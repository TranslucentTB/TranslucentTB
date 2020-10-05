#include "xamlcontentmanager.hpp"
#include <CoreWindow.h>
#include <winrt/Windows.UI.Core.h>

#include "../../ProgramLog/error/win32.hpp"
#include "window.hpp"

void XamlContentManager::InitializeXamlHosting()
{
	try
	{
		m_Manager = winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to initialize WindowsXamlManager");

	try
	{
		m_App = { };
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to load XAML resources");

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
