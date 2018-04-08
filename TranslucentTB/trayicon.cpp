#include "trayicon.hpp"
#include <random>
#include <shellapi.h>

#include "app.hpp"
#include "tray.hpp"

long TrayIcon::RegisterIcon(Window, WPARAM, LPARAM)
{
	Shell_NotifyIcon(NIM_ADD, &m_IconData);
	Shell_NotifyIcon(NIM_SETVERSION, &m_IconData);

	return 0;
}

MessageWindow::CALLBACKCOOKIE TrayIcon::RegisterTrayCallback(const std::function<long(Window, WPARAM, LPARAM)> &callback)
{
	return m_Window.RegisterCallback(m_IconData.uCallbackMessage, callback);
}

TrayIcon::TrayIcon(MessageWindow &window, wchar_t *iconResource, const unsigned int additionalFlags, const HINSTANCE &hInstance) :
	m_Window(window),
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
	m_IconData {
		sizeof(m_IconData),
		m_Window,
		101, // TODO: change this (randomly generated GUID?)
		NIF_ICON | NIF_TIP | NIF_MESSAGE | additionalFlags
	}
#pragma clang diagnostic pop
{
	LoadIconMetric(hInstance, iconResource, LIM_SMALL, &m_IconData.hIcon);
	wcscpy_s(m_IconData.szTip, App::NAME.c_str());

	std::random_device seed;
	std::mt19937 rng(seed());
	std::uniform_int_distribution<unsigned int> app_messages(WM_APP, 0xBFFF);
	m_IconData.uCallbackMessage = app_messages(rng);

	RegisterIcon();

	m_Cookie = m_Window.RegisterCallback(Tray::WM_TASKBARCREATED, std::bind(&TrayIcon::RegisterIcon, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

TrayIcon::~TrayIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &m_IconData);
	m_Window.UnregisterCallback(m_Cookie);
}
