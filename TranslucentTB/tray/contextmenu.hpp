#pragma once
#include "arch.h"
#include <string>
#include <windef.h>
#include <wil/resource.h>

#include "../localization.hpp"
#include "../../ProgramLog/error.hpp"
#include "window.hpp"

class ContextMenu {
private:
	wil::unique_hmenu m_Menu;

protected:
	inline void CheckItem(unsigned int id, bool state)
	{
		DWORD ret = CheckMenuItem(m_Menu.get(), id, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED));
		if (ret == -1)
		{
			MessagePrint(spdlog::level::info, L"Failed to check/uncheck menu item.");
		}
	}

	inline void EnableItem(unsigned int id, bool state)
	{
		BOOL ret = EnableMenuItem(m_Menu.get(), id, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED));
		if (ret == -1)
		{
			MessagePrint(spdlog::level::info, L"Failed to enable/disable menu item.");
		}
	}

	inline void CheckRadio(unsigned int first, unsigned int last, unsigned int id)
	{
		BOOL ret = CheckMenuRadioItem(m_Menu.get(), first, last, id, MF_BYCOMMAND);
		if (!ret)
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set menu radio item.");
		}
	}

	inline void SetText(unsigned int id, uint16_t new_text_resource)
	{
		MENUITEMINFO item_info = { sizeof(item_info), MIIM_STRING };

		std::wstring new_text(Localization::LoadLocalizedString(new_text_resource));
		item_info.dwTypeData = new_text.data();
		BOOL ret = SetMenuItemInfo(m_Menu.get(), id, false, &item_info);
		if (!ret)
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set menu item text.");
		}
	}

	inline void RemoveItem(unsigned int id)
	{
		BOOL ret = RemoveMenu(m_Menu.get(), id, MF_BYCOMMAND);
		if (!ret)
		{
			LastErrorHandle(spdlog::level::info, L"Failed to remove menu item.");
		}
	}

public:
	ContextMenu(const wchar_t *menuResource, HINSTANCE hInstance);

	virtual void RefreshMenu() = 0;
	virtual void ClickHandler(unsigned int id) = 0;

	void ShowAt(Window window, POINT pt);

	inline void ShowAtCursor(Window window)
	{
		if (POINT pt; GetCursorPos(&pt))
		{
			ShowAt(window, pt);
		}
		else
		{
			LastErrorHandle(spdlog::level::info, L"Failed to get cursor position.");
		}
	}
};
