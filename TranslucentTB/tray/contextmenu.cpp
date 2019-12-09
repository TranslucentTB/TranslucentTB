#include "contextmenu.hpp"

ContextMenu::ContextMenu(const wchar_t *menuResource, HINSTANCE hInstance)
{
	m_Menu.reset(LoadMenu(hInstance, menuResource));
	if (!m_Menu)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to load context menu.");
	}
}

void ContextMenu::ShowAt(Window window, POINT pt)
{
	SetLastError(NO_ERROR);
	const unsigned int item = TrackPopupMenu(GetSubMenu(m_Menu.get(), 0), TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, window, nullptr);
	if (item)
	{
		ClickHandler(item);
	}
	else
	{
		if (GetLastError() != NO_ERROR)
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to open context menu.");
		}
	}
}
