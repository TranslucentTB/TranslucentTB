#pragma once
#include "messagewindow.hpp"
#include <windef.h>

class TrayIcon {
protected:
	MessageWindow &m_Window;
	inline MessageWindow::CALLBACKCOOKIE RegisterTrayCallback(std::function<long(WPARAM, LPARAM)> callback)
	{
		return m_Window.RegisterCallback(m_IconData.uCallbackMessage, std::move(callback));
	}

private:
	static bool IsSystemLightThemeEnabled();

	NOTIFYICONDATA m_IconData;
	MessageWindow::CALLBACKCOOKIE m_TaskbarCreatedCookie;
	MessageWindow::CALLBACKCOOKIE m_SettingsChangedCookie;
	HINSTANCE m_hInstance;
	const wchar_t *m_BrightIconResource;
	const wchar_t *m_DarkIconResource;
	long RegisterIcon(...);
	long UpdateIcon(bool notify = false, ...);
	void DestroyIconHandle();

public:
	TrayIcon(MessageWindow &window, const wchar_t *brightIconResource, const wchar_t *darkIconResource, unsigned int additionalFlags = 0, HINSTANCE hInstance = GetModuleHandle(NULL));
	~TrayIcon();

	inline TrayIcon(const TrayIcon &) = delete;
	inline TrayIcon &operator =(const TrayIcon &) = delete;
};