#pragma once
#include <forward_list>
#include <functional>
#include <unordered_map>

#include "resource.h"
#include "window.hpp"
#include "windowclass.hpp"

class MessageWindow : public Window {
protected:
	using callback_t = std::function<long(WPARAM, LPARAM)>;

private:
	std::unordered_map<unsigned int, std::forward_list<std::pair<unsigned short, callback_t>>> m_CallbackMap;
	WindowClass m_WindowClass;

	LRESULT WindowProcedure(Window window, unsigned int uMsg, WPARAM wParam, LPARAM lParam);

public:
	static void RunMessageLoop();

	MessageWindow(const std::wstring &className, const std::wstring &windowName, HINSTANCE hInstance = GetModuleHandle(NULL), Window parent = Window::NullWindow, const wchar_t *iconResource = MAKEINTRESOURCE(MAINICON));
	using CALLBACKCOOKIE = unsigned long long;
	CALLBACKCOOKIE RegisterCallback(unsigned int message, callback_t callback);
	inline CALLBACKCOOKIE RegisterCallback(const std::wstring &message, callback_t callback)
	{
		return RegisterCallback(RegisterWindowMessage(message.c_str()), std::move(callback));
	}
	void UnregisterCallback(CALLBACKCOOKIE cookie);
	~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;
};