#include "contextmenu.hpp"
#include "../../ProgramLog/error.hpp"

ContextMenu::ContextMenu(MessageWindow &window, const wchar_t *menuResource, HINSTANCE hInstance) : m_Window(window)
{
	m_Menu.reset(LoadMenu(hInstance, menuResource));
	if (!m_Menu)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to load context menu.");
	}

	m_MenuInitCookie = m_Window.RegisterCallback(WM_INITMENU, [this](...)
	{
		Updater updater(m_Menu.get());
		for (const auto &[_, refreshFunction]: m_RefreshFunctions)
		{
			refreshFunction(updater);
		}
		return 0;
	});
}

void ContextMenu::ShowAtCursor()
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

	SetLastError(NO_ERROR);
	const unsigned int item = TrackPopupMenu(GetSubMenu(m_Menu.get(), 0), TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, m_Window, nullptr);
	if (!item && GetLastError() != NO_ERROR)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to open context menu.");
		return;
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

ContextMenu::~ContextMenu()
{
	m_Window.UnregisterCallback(m_MenuInitCookie);
}
