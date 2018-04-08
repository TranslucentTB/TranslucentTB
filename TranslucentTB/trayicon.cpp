#include "trayicon.hpp"
#include <random>
#include <shellapi.h>

#include "app.hpp"
#include "tray.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"

long TrayIcon::RegisterIcon(Window, WPARAM, LPARAM)
{
	if (!Shell_NotifyIcon(NIM_ADD, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon addition.");
	}
	if (!Shell_NotifyIcon(NIM_SETVERSION, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon version.");
	}

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
	ErrorHandle(LoadIconMetric(hInstance, iconResource, LIM_SMALL, &m_IconData.hIcon), Error::Level::Log, L"Failed to load tray icon.");
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
	if (!Shell_NotifyIcon(NIM_DELETE, &m_IconData))
	{
		Log::OutputMessage(L"Failed to notify shell of icon deletion.");
	}
	m_Window.UnregisterCallback(m_Cookie);
}
