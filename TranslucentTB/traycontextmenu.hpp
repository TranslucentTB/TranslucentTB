#pragma once
#include "arch.h"
#include <algorithm>
#include <forward_list>
#include <string>
#include <type_traits>
#include <windef.h>

#include "trayicon.hpp"
#include "util.hpp"
#include "win32.hpp"

class TrayContextMenu : public TrayIcon {
protected:
	using callback_t = std::function<void()>;

private:
	HMENU m_Menu;
	std::unordered_map<unsigned int, std::forward_list<std::pair<unsigned short, callback_t>>> m_MenuCallbackMap;
	long TrayCallback(WPARAM, LPARAM);
	MessageWindow::CALLBACKCOOKIE m_TrayCallbackCookie;
	MessageWindow::CALLBACKCOOKIE m_MenuInitCookie;

	std::vector<std::function<void()>> m_RefreshFunctions;

public:
	TrayContextMenu(MessageWindow &window, wchar_t *brightIconResource, wchar_t *darkIconResource, wchar_t *menuResource, const HINSTANCE &hInstance = GetModuleHandle(NULL));

	using MENUCALLBACKCOOKIE = unsigned long long;

	inline MENUCALLBACKCOOKIE RegisterContextMenuCallback(unsigned int item, const callback_t &callback)
	{
		unsigned short secret = Util::GetRandomNumber<unsigned short>();
		m_MenuCallbackMap[item].emplace_front(std::make_pair(secret, callback));

		return (static_cast<MENUCALLBACKCOOKIE>(secret) << 32) + item;
	}

	inline void UnregisterContextMenuCallback(MENUCALLBACKCOOKIE cookie)
	{
		unsigned int item = cookie & 0xFFFFFFFF;
		unsigned short secret = (cookie >> 32) & 0xFFFF;

		m_MenuCallbackMap[item].remove_if([&secret](const std::pair<unsigned short, callback_t> &pair) -> bool
		{
			return pair.first == secret;
		});
	}

	enum BoolBindingEffect {
		Toggle,
		ControlsEnabled
	};

	inline static void RefreshBool(unsigned int item, HMENU menu, const bool &value, BoolBindingEffect effect)
	{
		if (effect == Toggle)
		{
			CheckMenuItem(menu, item, MF_BYCOMMAND | (value ? MF_CHECKED : MF_UNCHECKED));
		}
		else if (effect == ControlsEnabled)
		{
			EnableMenuItem(menu, item, MF_BYCOMMAND | (value ? MF_ENABLED : MF_GRAYED));
		}
	}

	inline static void RefreshEnum(HMENU menu, unsigned int first, unsigned int last, unsigned int position)
	{
		CheckMenuRadioItem(menu, first, last, position, MF_BYCOMMAND);
	}

	inline static void ChangeItemText(HMENU menu, const uint32_t &item, std::wstring &&new_text)
	{
		MENUITEMINFO item_info = { sizeof(item_info), MIIM_STRING };

		item_info.dwTypeData = new_text.data();
		SetMenuItemInfo(menu, item, false, &item_info);
	}

	inline void BindBool(unsigned int item, bool &value, BoolBindingEffect effect)
	{
		if (effect == Toggle)
		{
			RegisterContextMenuCallback(item, std::bind(&Util::InvertBool, std::ref(value)));
		}

		m_RefreshFunctions.push_back(std::bind(&TrayContextMenu::RefreshBool, item, m_Menu, std::ref(value), effect));
	}

	template<class T>
	inline void BindEnum(T &value, const std::unordered_map<T, unsigned int> &map)
	{
		static_assert(std::is_enum_v<T>, "T is not an enum.");
		for (const auto &[item_value, item] : map)
		{
			RegisterContextMenuCallback(item, std::bind(&Util::UpdateValue<T>, std::ref(value), std::ref(item_value)));
		}

		const auto [min_p, max_p] = std::minmax_element(map.begin(), map.end(), Util::map_value_compare<T, unsigned int>());
		unsigned int min = min_p->second;
		unsigned int max = max_p->second;

		m_RefreshFunctions.emplace_back([this, min, max, &value, &map]
		{
			RefreshEnum(m_Menu, min, max, map.at(value));
		});
	}

	inline void BindColor(unsigned int item, uint32_t &color)
	{
		RegisterContextMenuCallback(item, std::bind(&win32::PickColor, std::ref(color)));
	}

	inline void RegisterCustomRefresh(const std::function<void(HMENU menu)> &function)
	{
		m_RefreshFunctions.push_back(std::bind(function, m_Menu));
	}

	~TrayContextMenu();
};