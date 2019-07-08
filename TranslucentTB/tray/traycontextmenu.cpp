#include "traycontextmenu.hpp"
#include "../ttberror.hpp"
#include "../ttblog.hpp"

long TrayContextMenu::TrayCallback(WPARAM, LPARAM lParam)
{
	if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
	{
		POINT pt;
		if (!GetCursorPos(&pt))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to get cursor position.");
		}

		if (!SetForegroundWindow(m_Window))
		{
			MessagePrint(spdlog::level::info, L"Failed to set window as foreground window.");
		}

		SetLastError(0);
		unsigned int item = TrackPopupMenu(GetSubMenu(m_Menu, 0), TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, m_Window, nullptr);
		if (!item && GetLastError() != 0)
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to open context menu.");
			return 0;
		}

		const auto &callbackMap = m_MenuCallbackMap[item];
		if (!callbackMap.empty())
		{
			for (const auto &[_, callback] : callbackMap)
			{
				callback();
			}
		}
	}
	return 0;
}

TrayContextMenu::TrayContextMenu(MessageWindow &window, const wchar_t *iconResource, const wchar_t *menuResource, HINSTANCE hInstance) :
	TrayIcon(window, iconResource, 0, hInstance)
{
	m_Menu = LoadMenu(hInstance, menuResource);
	if (!m_Menu)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to load context menu.");
	}

	m_TrayCallbackCookie = RegisterTrayCallback(std::bind(&TrayContextMenu::TrayCallback, this, std::placeholders::_1, std::placeholders::_2));
	m_MenuInitCookie = m_Window.RegisterCallback(WM_INITMENU, [this](...)
	{
		Updater updater(m_Menu);
		for (const auto &[_, refreshFunction]: m_RefreshFunctions)
		{
			refreshFunction(updater);
		}
		return 0;
	});
}

TrayContextMenu::~TrayContextMenu()
{
	m_Window.UnregisterCallback(m_MenuInitCookie);
	m_Window.UnregisterCallback(m_TrayCallbackCookie);
	if (!DestroyMenu(m_Menu))
	{
		LastErrorHandle(spdlog::level::info, L"Failed to destroy context menu.");
	}
}