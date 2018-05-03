#pragma once
#include "messagewindow.hpp"
#include <windef.h>

class TrayIcon {

protected:
	MessageWindow &m_Window;
	MessageWindow::CALLBACKCOOKIE RegisterTrayCallback(const std::function<long(Window, WPARAM, LPARAM)> &callback);

private:
	NOTIFYICONDATA m_IconData;
	long RegisterIcon(Window = Window::NullWindow, WPARAM = NULL, LPARAM = NULL);
	MessageWindow::CALLBACKCOOKIE m_Cookie;

public:
	TrayIcon(MessageWindow &window, wchar_t *iconResource, const unsigned int additionalFlags = 0, const HINSTANCE &hInstance = GetModuleHandle(NULL));
	~TrayIcon();

	inline TrayIcon(const TrayIcon &) = delete;
	inline TrayIcon &operator =(const TrayIcon &) = delete;
};