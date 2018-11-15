#pragma once
#include "messagewindow.hpp"
#include <windef.h>

class TrayIcon {
protected:
	MessageWindow &m_Window;
	inline MessageWindow::CALLBACKCOOKIE RegisterTrayCallback(const std::function<long(WPARAM, LPARAM)> &callback)
	{
		return m_Window.RegisterCallback(m_IconData.uCallbackMessage, callback);
	}

private:
	static bool IsSystemLightThemeEnabled();

	NOTIFYICONDATA m_IconData;
	MessageWindow::CALLBACKCOOKIE m_TaskbarCreatedCookie;
	MessageWindow::CALLBACKCOOKIE m_SettingsChangedCookie;
	HINSTANCE m_hInstance;
	wchar_t *m_BrightIconResource;
	wchar_t *m_DarkIconResource;
	long RegisterIcon(...);
	long UpdateIcon(bool notify = false, ...);
	void DestroyIconHandle();

public:
	TrayIcon(MessageWindow &window, wchar_t *brightIconResource, wchar_t *darkIconResource, const unsigned int additionalFlags = 0, const HINSTANCE &hInstance = GetModuleHandle(NULL));
	~TrayIcon();

	inline TrayIcon(const TrayIcon &) = delete;
	inline TrayIcon &operator =(const TrayIcon &) = delete;
};