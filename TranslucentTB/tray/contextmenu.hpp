#pragma once
#include "arch.h"
#include <string>
#include <utility>
#include <windef.h>

#include "../localization.hpp"
#include "../windows/messagewindow.hpp"
#include "util/others.hpp"
#include "util/random.hpp"
#include "../log/ttberror.hpp"
#include "../win32.hpp"

class ContextMenu {
public:
	using MENUCALLBACKCOOKIE = unsigned long long;
	using MENUREFRESHCOOKIE = unsigned int;

	class Updater {
	private:
		HMENU m_hMenu;
		inline explicit Updater(HMENU hmenu) : m_hMenu(hmenu) { }
		friend class ContextMenu;

	public:
		inline void CheckItem(unsigned int id, bool state)
		{
			DWORD ret = CheckMenuItem(m_hMenu, id, MF_BYCOMMAND | (state ? MF_CHECKED : MF_UNCHECKED));
			if (ret == -1)
			{
				MessagePrint(spdlog::level::info, L"Failed to check/uncheck menu item.");
			}
		}

		inline void EnableItem(unsigned int id, bool state)
		{
			BOOL ret = EnableMenuItem(m_hMenu, id, MF_BYCOMMAND | (state ? MF_ENABLED : MF_GRAYED));
			if (ret == -1)
			{
				MessagePrint(spdlog::level::info, L"Failed to enable/disable menu item.");
			}
		}

		inline void CheckRadio(unsigned int first, unsigned int last, unsigned int id)
		{
			BOOL ret = CheckMenuRadioItem(m_hMenu, first, last, id, MF_BYCOMMAND);
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
			BOOL ret = SetMenuItemInfo(m_hMenu, id, false, &item_info);
			if (!ret)
			{
				LastErrorHandle(spdlog::level::info, L"Failed to set menu item text.");
			}
		}

		inline void RemoveItem(unsigned int id)
		{
			BOOL ret = RemoveMenu(m_hMenu, id, MF_BYCOMMAND);
			if (!ret)
			{
				LastErrorHandle(spdlog::level::info, L"Failed to remove menu item.");
			}
		}
	};

private:
	using callback_t = std::function<void()>;
	using updater_t = std::function<void(Updater)>;

	MessageWindow &m_Window;
	HMENU m_Menu;
	MessageWindow::CALLBACKCOOKIE m_TrayCallbackCookie;
	MessageWindow::CALLBACKCOOKIE m_MenuInitCookie;
	std::unordered_map<unsigned int, std::unordered_map<unsigned short, callback_t>> m_MenuCallbackMap;
	std::unordered_map<unsigned int, updater_t> m_RefreshFunctions;

public:
	ContextMenu(MessageWindow &window, const wchar_t *menuResource, HINSTANCE hInstance = GetModuleHandle(nullptr));

	// Technically safe to copy but most certainly a bug.
	inline ContextMenu(const ContextMenu &other) = delete;
	inline ContextMenu &operator =(const ContextMenu &other) = delete;

	void ShowAtCursor();

	inline MENUCALLBACKCOOKIE RegisterCallback(unsigned int item, callback_t callback)
	{
		auto &callbackMap = m_MenuCallbackMap[item];

		const auto secret = Util::GetSecret(callbackMap);

		callbackMap[secret] = std::move(callback);

		return (static_cast<MENUCALLBACKCOOKIE>(secret) << 32) + item;
	}

	inline void UnregisterCallback(MENUCALLBACKCOOKIE cookie)
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

	~ContextMenu();
};
