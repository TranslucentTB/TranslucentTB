#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

#include "window.hpp"
#include "windowclass.hpp"

#include "resource.h"

class MessageWindow : public Window {

private:
	typedef std::function<long(Window, WPARAM, LPARAM)> m_CallbackFunction;
	std::unordered_map<unsigned int, std::vector<std::pair<unsigned short, m_CallbackFunction>>> m_CallbackMap;
	WindowClass m_WindowClass;

	static long CALLBACK m_StaticCallback(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam);

public:
	MessageWindow(const std::wstring &className, const std::wstring &windowName, const HINSTANCE &hInstance = GetModuleHandle(NULL), const wchar_t *iconResource = MAKEINTRESOURCE(MAINICON));
	typedef unsigned long long CALLBACKCOOKIE;
	CALLBACKCOOKIE RegisterCallback(unsigned int message, const m_CallbackFunction &callback);
	bool UnregisterCallback(CALLBACKCOOKIE cookie);
	~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;
};