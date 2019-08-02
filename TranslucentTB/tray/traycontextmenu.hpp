#pragma once
#include "arch.h"
#include <string>
#include <utility>
#include <windef.h>

#include "../localization.hpp"
#include "trayicon.hpp"
#include "util/others.hpp"
#include "util/random.hpp"
#include "../win32.hpp"

class TrayContextMenu : public TrayIcon {
public:
	using MENUCALLBACKCOOKIE = unsigned long long;
	using MENUREFRESHCOOKIE = unsigned int;

	class Updater {
	private:
		HMENU m_hMenu;
		inline explicit Updater(HMENU hmenu) : m_hMenu(hmenu) { }
		friend class TrayContextMenu;

	public:
		inline void CheckItem(unsigned int id, bool state)
		{
			CheckMenuItem(m_hMenu, id, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED));
		}

		inline void EnableItem(unsigned int id, bool state)
		{
			EnableMenuItem(m_hMenu, id, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED));
		}

		inline void CheckRadio(unsigned int first, unsigned int last, unsigned int id)
		{
			CheckMenuRadioItem(m_hMenu, first, last, id, MF_BYCOMMAND);
		}

		inline void SetText(unsigned int id, uint16_t new_text_resource)
		{
			MENUITEMINFO item_info = { sizeof(item_info), MIIM_STRING };

			std::wstring new_text(Localization::LoadLocalizedString(new_text_resource));
			item_info.dwTypeData = new_text.data();
			SetMenuItemInfo(m_hMenu, id, false, &item_info);
		}

		inline void RemoveItem(unsigned int id)
		{
			RemoveMenu(m_hMenu, id, MF_BYCOMMAND);
		}
	};

private:
	using callback_t = std::function<void()>;
	using updater_t = std::function<void(Updater)>;
	
	long TrayCallback(WPARAM, LPARAM);

	HMENU m_Menu;
	MessageWindow::CALLBACKCOOKIE m_TrayCallbackCookie;
	MessageWindow::CALLBACKCOOKIE m_MenuInitCookie;
	std::unordered_map<unsigned int, std::unordered_map<unsigned short, callback_t>> m_MenuCallbackMap;
	std::unordered_map<unsigned int, updater_t> m_RefreshFunctions;

public:
	TrayContextMenu(MessageWindow &window, const wchar_t *iconResource, const wchar_t *menuResource, HINSTANCE hInstance = GetModuleHandle(nullptr));

	inline MENUCALLBACKCOOKIE RegisterContextMenuCallback(unsigned int item, callback_t callback)
	{
		auto &callbackMap = m_MenuCallbackMap[item];

		const auto secret = Util::GetSecret(callbackMap);

		callbackMap[secret] = std::move(callback);

		return (static_cast<MENUCALLBACKCOOKIE>(secret) << 32) + item;
	}

	inline void UnregisterContextMenuCallback(MENUCALLBACKCOOKIE cookie)
	{
		unsigned int item = cookie & 0xFFFFFFFF;
		unsigned short secret = (cookie >> 32) & 0xFFFF;

		m_MenuCallbackMap[item].erase(secret);
	}

	inline MENUREFRESHCOOKIE RegisterCustomRefresh(updater_t function)
	{
		const auto secret = Util::GetSecret(m_RefreshFunctions);
		m_RefreshFunctions[secret] = std::move(function);
		return secret;
	}

	inline void UnregisterCustomRefresh(MENUREFRESHCOOKIE cookie)
	{
		m_RefreshFunctions.erase(cookie);
	}

	inline Updater Update()
	{
		return Updater(m_Menu);
	}

	~TrayContextMenu();
};
