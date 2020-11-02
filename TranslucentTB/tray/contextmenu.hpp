#pragma once
#include "arch.h"
#include <windef.h>
#include <wil/resource.h>

#include "../../ProgramLog/error/win32.hpp"
#include "util/null_terminated_string_view.hpp"
#include "window.hpp"

class ContextMenu {
private:
	wil::unique_hmenu m_Menu;

protected:
	inline void CheckItem(unsigned int id, bool state) { CheckItem(m_Menu.get(), id, state); }
	inline void EnableItem(unsigned int id, bool state) { EnableItem(m_Menu.get(), id, state); }
	inline void CheckRadio(unsigned int first, unsigned int last, unsigned int id) { CheckRadio(m_Menu.get(), first, last, id); }
	inline void SetText(unsigned int id, Util::null_terminated_wstring_view new_text) { SetText(m_Menu.get(), id, new_text); }
	inline void RemoveItem(unsigned int id) { RemoveItem(m_Menu.get(), id); }
	inline void SetDefaultItem(unsigned int id) { SetDefaultItem(m_Menu.get(), id); }

public:
	inline static void CheckItem(HMENU menu, unsigned int id, bool state)
	{
		if (CheckMenuItem(menu, id, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED)) == -1)
		{
			MessagePrint(spdlog::level::info, L"Failed to check/uncheck menu item.");
		}
	}

	inline static void EnableItem(HMENU menu, unsigned int id, bool state)
	{
		if (EnableMenuItem(menu, id, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED)) == -1)
		{
			MessagePrint(spdlog::level::info, L"Failed to enable/disable menu item.");
		}
	}

	inline static void CheckRadio(HMENU menu, unsigned int first, unsigned int last, unsigned int id)
	{
		if (!CheckMenuRadioItem(menu, first, last, id, MF_BYCOMMAND))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set menu radio item.");
		}
	}

	inline static void SetText(HMENU menu, unsigned int id, Util::null_terminated_wstring_view new_text)
	{
		const MENUITEMINFO item_info = {
			.cbSize = sizeof(item_info),
			.fMask = MIIM_STRING,
			.dwTypeData = const_cast<wchar_t *>(new_text.c_str())
		};

		if (!SetMenuItemInfo(menu, id, false, &item_info))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set menu item text.");
		}
	}

	inline static void RemoveItem(HMENU menu, unsigned int id)
	{
		if (!RemoveMenu(menu, id, MF_BYCOMMAND))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to remove menu item.");
		}
	}

	inline static void SetDefaultItem(HMENU menu, unsigned int id)
	{
		if (!SetMenuDefaultItem(menu, id, false))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to set default menu item.");
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
