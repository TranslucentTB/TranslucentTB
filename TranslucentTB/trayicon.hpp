#pragma once
#include "messagewindow.hpp"
#include <windef.h>

class TrayIcon : public MessageWindow {

private:
	NOTIFYICONDATA m_IconData;
	long RegisterIcon(Window = nullptr, WPARAM = NULL, LPARAM = NULL);

public:
	TrayIcon(const std::wstring &classname, wchar_t *iconResource, const unsigned int additionalFlags = 0, const HINSTANCE &hInstance = GetModuleHandle(NULL));
	CALLBACKCOOKIE RegisterTrayCallback(const m_CallbackFunction &callback);
	~TrayIcon();

	inline TrayIcon(const TrayIcon &) = delete;
	inline TrayIcon &operator =(const TrayIcon &) = delete;
};