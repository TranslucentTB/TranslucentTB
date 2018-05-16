#pragma once
#include "arch.h"
#include <string>
#include <type_traits>
#include <windef.h>

#include "trayicon.hpp"

class TrayContextMenu : public TrayIcon {

protected:
	typedef std::function<void(unsigned int)> m_MenuCallbackFunction;

private:
	HMENU m_Menu;
	std::unordered_map<unsigned int, std::vector<std::pair<unsigned short, m_MenuCallbackFunction>>> m_MenuCallbackMap;
	long TrayCallback(Window, WPARAM, LPARAM);
	MessageWindow::CALLBACKCOOKIE m_Cookie;

	std::vector<std::function<void()>> m_RefreshFunctions;

public:
	TrayContextMenu(MessageWindow &window, wchar_t *iconResource, wchar_t *menuResource, const HINSTANCE &hInstance = GetModuleHandle(NULL));

	typedef unsigned long long MENUCALLBACKCOOKIE;
	MENUCALLBACKCOOKIE RegisterContextMenuCallback(unsigned int item, const m_MenuCallbackFunction &callback);
	bool UnregisterContextMenuCallback(MENUCALLBACKCOOKIE cookie);

	enum BoolBindingEffect {
		Toggle,
		ControlsEnabled
	};

	static void RefreshBool(unsigned int item, HMENU menu, const bool &value, BoolBindingEffect effect);
	static void RefreshEnum(HMENU menu, unsigned int first, unsigned int last, unsigned int position);

	void BindBool(unsigned int item, bool &value, BoolBindingEffect effect);

	template<class T>
	inline void BindEnum(unsigned int first, unsigned int last, T &value, const std::unordered_map<T, unsigned int> &map)
	{
		static_assert(std::is_enum<T>::value, "T is not an enum.");
		for (const auto &button_pair : map)
		{
			RegisterContextMenuCallback(button_pair.second, std::bind(&Util::UpdateValue<T>, std::ref(value), button_pair.first));
		}

		m_RefreshFunctions.push_back([=, &value, &map]() {
			RefreshEnum(m_Menu, first, last, map.at(value));
		});
	}

	void RegisterCustomRefresh(const std::function<void(HMENU menu)> &function);

	~TrayContextMenu();
};