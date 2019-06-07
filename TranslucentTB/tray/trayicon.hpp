#pragma once
#include "../windows/messagewindow.hpp"
#include <windef.h>
#include <wil/resource.h>

class TrayIcon {
protected:
	MessageWindow &m_Window;

private:
	NOTIFYICONDATA m_IconData;
	MessageWindow::CALLBACKCOOKIE m_TaskbarCreatedCookie;
	MessageWindow::CALLBACKCOOKIE m_DpiChangedCookie;
	HINSTANCE m_hInstance;
	const wchar_t *m_IconResource;
	wil::unique_hicon m_Icon;
	long RegisterIcon(...);
	void LoadIcon();
	long UpdateIcon(...);

public:
	TrayIcon(MessageWindow &window, const wchar_t *iconResource, unsigned int additionalFlags = 0, HINSTANCE hInstance = GetModuleHandle(nullptr));

	inline MessageWindow::CALLBACKCOOKIE RegisterTrayCallback(std::function<long(WPARAM, LPARAM)> callback)
	{
		return m_Window.RegisterCallback(m_IconData.uCallbackMessage, std::move(callback));
	}

	inline void SetIcon(const wchar_t *iconResource)
	{
		m_IconResource = iconResource;
		UpdateIcon();
	}

	inline MessageWindow &GetWindow()
	{
		return m_Window;
	}

	~TrayIcon();

	inline TrayIcon(const TrayIcon &) = delete;
	inline TrayIcon &operator =(const TrayIcon &) = delete;
};