#include "traycontextmenu.hpp"
#include <random>

#include "util.hpp"

long TrayContextMenu::TrayCallback(Window, WPARAM, LPARAM lParam)
{
	if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
	{
		for (const auto &refreshFunction : m_RefreshFunctions)
		{
			refreshFunction();
		}

		POINT pt;
		GetCursorPos(&pt);
		SetForegroundWindow(m_Window);

		unsigned int item = TrackPopupMenu(GetSubMenu(m_Menu, 0), TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, m_Window, NULL);

		const auto &callbackVector = m_MenuCallbackMap[item];
		if (callbackVector.size() > 0)
		{
			for (const auto &callbackPair : callbackVector)
			{
				callbackPair.second(item);
			}
		}
	}
	return 0;
}

TrayContextMenu::TrayContextMenu(MessageWindow &window, wchar_t *iconResource, wchar_t *menuResource, const HINSTANCE &hInstance) :
	TrayIcon(window, iconResource, 0, hInstance)
{
	m_Menu = LoadMenu(hInstance, menuResource);
	m_Cookie = RegisterTrayCallback(std::bind(&TrayContextMenu::TrayCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

TrayContextMenu::MENUCALLBACKCOOKIE TrayContextMenu::RegisterContextMenuCallback(unsigned int item, const m_MenuCallbackFunction &callback)
{
	std::random_device seed;
	std::mt19937 rng(seed());
	std::uniform_int_distribution<unsigned short> ushort_values(0, USHRT_MAX);

	unsigned short secret = ushort_values(rng);
	m_MenuCallbackMap[item].push_back(std::make_pair(secret, callback));

	return (static_cast<MENUCALLBACKCOOKIE>(secret) << 32) & item;
}

bool TrayContextMenu::UnregisterContextMenuCallback(MENUCALLBACKCOOKIE cookie)
{
	unsigned int item = cookie & 0xFFFFFFFF;
	unsigned short secret = (cookie >> 32) & 0xFFFF;

	auto &callbackVector = m_MenuCallbackMap[item];
	for (auto &callbackPair : callbackVector)
	{
		if (callbackPair.first == secret)
		{
			std::swap(callbackPair, callbackVector.back());
			callbackVector.pop_back();
			return true;
		}
	}

	return false;
}

void TrayContextMenu::RefreshBool(unsigned int item, HMENU menu, const bool &value, BoolBindingEffect effect)
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

void TrayContextMenu::RefreshEnum(HMENU menu, unsigned int first, unsigned int last, unsigned int position)
{
	CheckMenuRadioItem(menu, first, last, position, MF_BYCOMMAND);
}

void TrayContextMenu::BindBool(unsigned int item, bool &value, BoolBindingEffect effect)
{
	if (effect == Toggle)
	{
		RegisterContextMenuCallback(item, std::bind(&Util::InvertBool, std::ref(value), std::placeholders::_1));
	}

	m_RefreshFunctions.push_back(std::bind(&TrayContextMenu::RefreshBool, item, m_Menu, std::ref(value), effect));
}

void TrayContextMenu::RegisterCustomRefresh(const std::function<void(HMENU menu)> &function)
{
	m_RefreshFunctions.push_back(std::bind(function, m_Menu));
}

TrayContextMenu::~TrayContextMenu()
{
	m_Window.UnregisterCallback(m_Cookie);
}