#pragma once
#include "arch.h"
#include <string>
#include <windef.h>
#include <wil/resource.h>

#include "../localization.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "window.hpp"

class ContextMenu {
private:
	wil::unique_hmenu m_Menu;

protected:
	inline void CheckItem(unsigned int id, bool state) { CheckItem(m_Menu.get(), id, state); }
	inline void EnableItem(unsigned int id, bool state) { EnableItem(m_Menu.get(), id, state); }
	inline void CheckRadio(unsigned int first, unsigned int last, unsigned int id) { CheckRadio(m_Menu.get(), first, last, id); }
	inline void SetText(unsigned int id, uint16_t new_text_resource) { SetText(m_Menu.get(), id, new_text_resource); }
	inline void RemoveItem(unsigned int id) { RemoveItem(m_Menu.get(), id); }

public:
	inline static void CheckItem(HMENU menu, unsigned int id, bool state)
	{
		const DWORD ret = CheckMenuItem(menu, id, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED));
		if (ret == -1)
		{
			MessagePrint(spdlog::level::info, L"Failed to check/uncheck menu item.");
		}
	}

	inline static void EnableItem(HMENU menu, unsigned int id, bool state)
	{
		const BOOL ret = EnableMenuItem(menu, id, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED));
		if (ret == -1)
		{
			MessagePrint(spdlog::level::info, L"Failed to enable/disable menu item.");
		}
	}

	inline static void CheckRadio(HMENU menu, unsigned int first, unsigned int last, unsigned int id)
	{
		const BOOL ret = CheckMenuRadioItem(menu, first, last, id, MF_BYCOMMAND);
		if (!ret)
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set menu radio item.");
		}
	}

	inline static void SetText(HMENU menu, unsigned int id, uint16_t new_text_resource)
	{
		std::wstring new_text(Localization::LoadLocalizedString(new_text_resource));
		const MENUITEMINFO item_info = {
			.cbSize = sizeof(item_info),
			.fMask = MIIM_STRING,
			.dwTypeData = new_text.data()
		};

		BOOL ret = SetMenuItemInfo(menu, id, false, &item_info);
		if (!ret)
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set menu item text.");
		}
	}

	inline static void RemoveItem(HMENU menu, unsigned int id)
	{
		const BOOL ret = RemoveMenu(menu, id, MF_BYCOMMAND);
		if (!ret)
		{
			LastErrorHandle(spdlog::level::info, L"Failed to remove menu item.");
		}
	}

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
