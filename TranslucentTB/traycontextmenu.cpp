#include "traycontextmenu.hpp"

#include "ttberror.hpp"
#include "ttblog.hpp"

long TrayContextMenu::TrayCallback(WPARAM, LPARAM lParam)
{
	if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
	{
		for (const auto &refreshFunction : m_RefreshFunctions)
		{
			refreshFunction();
		}

		POINT pt;
		if (!GetCursorPos(&pt))
		{
			LastErrorHandle(Error::Level::Log, L"Failed to get cursor position.");
		}

		if (!SetForegroundWindow(m_Window))
		{
			Log::OutputMessage(L"Failed to set window as foreground window.");
		}

		SetLastError(0);
		unsigned int item = TrackPopupMenu(GetSubMenu(m_Menu, 0), TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, m_Window, NULL);
		if (!item && GetLastError() != 0)
		{
			LastErrorHandle(Error::Level::Log, L"Failed to open context menu.");
			return 0;
		}

		const auto &callbackList = m_MenuCallbackMap[item];
		if (!callbackList.empty())
		{
			for (const auto &[_, callback] : callbackList)
			{
				callback();
			}
		}
	}
	return 0;
}

TrayContextMenu::TrayContextMenu(MessageWindow &window, wchar_t *iconResource, wchar_t *menuResource, const HINSTANCE &hInstance) :
	TrayIcon(window, iconResource, 0, hInstance)
{
	m_Menu = LoadMenu(hInstance, menuResource);
	if (!m_Menu)
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to load context menu.");
	}

	m_Cookie = RegisterTrayCallback(std::bind(&TrayContextMenu::TrayCallback, this, std::placeholders::_1, std::placeholders::_2));
}

TrayContextMenu::~TrayContextMenu()
{
	m_Window.UnregisterCallback(m_Cookie);
	if (!DestroyMenu(m_Menu))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy menu");
	}
}