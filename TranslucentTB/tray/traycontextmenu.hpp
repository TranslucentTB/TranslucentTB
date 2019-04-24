#pragma once
#include "arch.h"
#include <algorithm>
#include <string>
#include <type_traits>
#include <windef.h>

#include "trayicon.hpp"
#include "util/others.hpp"
#include "util/random.hpp"
#include "../win32.hpp"

class TrayContextMenu : public TrayIcon {
protected:
	using callback_t = std::function<void()>;

public:
	enum BoolBindingEffect {
		Toggle,
		ControlsEnabled
	};

private:
	template<typename T>
	inline static void UpdateValue(T &toupdate, const T &newvalue)
	{
		toupdate = newvalue;
	}

	inline static void InvertBool(bool &value)
	{
		value = !value;
	}

	inline static void RefreshBool(unsigned int item, HMENU menu, bool value, BoolBindingEffect effect)
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

	inline static void ChangeItemText(HMENU menu, unsigned int item, std::wstring &&new_text)
	{
		MENUITEMINFO item_info = { sizeof(item_info), MIIM_STRING };

		item_info.dwTypeData = new_text.data();
		SetMenuItemInfo(menu, item, false, &item_info);
	}

	HMENU m_Menu;
	std::unordered_map<unsigned int, std::unordered_map<unsigned short, callback_t>> m_MenuCallbackMap;
	long TrayCallback(WPARAM, LPARAM);
	MessageWindow::CALLBACKCOOKIE m_TrayCallbackCookie;
	MessageWindow::CALLBACKCOOKIE m_MenuInitCookie;

	std::vector<std::function<void()>> m_RefreshFunctions;

public:
	class ContextMenuUpdater {
	private:
		HMENU m_hMenu;
		inline ContextMenuUpdater(HMENU hmenu) : m_hMenu(hmenu) { }
		friend class TrayContextMenu;

	public:
		inline void CheckItem(unsigned int id, bool state)
		{
			RefreshBool(id, m_hMenu, state, BoolBindingEffect::Toggle);
		}

		inline void EnableItem(unsigned int id, bool state)
		{
			RefreshBool(id, m_hMenu, state, BoolBindingEffect::ControlsEnabled);
		}

		inline void SetText(unsigned int id, std::wstring &&new_text)
		{
			ChangeItemText(m_hMenu, id, std::forward<std::wstring>(new_text));
		}
	};

	TrayContextMenu(MessageWindow &window, const wchar_t *brightIconResource, const wchar_t *darkIconResource, const wchar_t *menuResource, HINSTANCE hInstance = GetModuleHandle(NULL));

	using MENUCALLBACKCOOKIE = unsigned long long;

	inline MENUCALLBACKCOOKIE RegisterContextMenuCallback(unsigned int item, callback_t callback)
	{
		auto &callbackMap = m_MenuCallbackMap[item];

		unsigned short secret;
		do
		{
			secret = Util::GetRandomNumber<unsigned short>();
		}
		while (callbackMap.contains(secret));

		callbackMap[secret] = std::move(callback);

		return (static_cast<MENUCALLBACKCOOKIE>(secret) << 32) + item;
	}

	inline void UnregisterContextMenuCallback(MENUCALLBACKCOOKIE cookie)
	{
		unsigned int item = cookie & 0xFFFFFFFF;
		unsigned short secret = (cookie >> 32) & 0xFFFF;

		m_MenuCallbackMap[item].erase(secret);
	}

	inline void BindBool(unsigned int item, bool &value, BoolBindingEffect effect)
	{
		if (effect == Toggle)
		{
			RegisterContextMenuCallback(item, std::bind(&InvertBool, std::ref(value)));
		}

		m_RefreshFunctions.push_back(std::bind(&TrayContextMenu::RefreshBool, item, m_Menu, std::ref(value), effect));
	}

	template<class T>
	inline void BindEnum(T &value, const std::unordered_map<T, unsigned int> &map)
	{
		static_assert(std::is_enum_v<T>, "T is not an enum.");
		for (const auto &[item_value, item] : map)
		{
			RegisterContextMenuCallback(item, std::bind(&UpdateValue<T>, std::ref(value), std::cref(item_value)));
		}

		const auto [min_p, max_p] = std::minmax_element(map.begin(), map.end(), Util::map_value_compare<T, unsigned int>());

		m_RefreshFunctions.push_back([this, min = min_p->second, max = max_p->second, &value, &map]
		{
			RefreshEnum(m_Menu, min, max, map.at(value));
		});
	}

	inline void BindColor(unsigned int item, COLORREF &color)
	{
		// TODO: no op
	}

	inline void RegisterCustomRefresh(std::function<void(ContextMenuUpdater)> function)
	{
		m_RefreshFunctions.push_back(std::bind(std::move(function), ContextMenuUpdater(m_Menu)));
	}

	~TrayContextMenu();
};