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

	LRESULT WindowProcedure(const Window &window, unsigned int uMsg, WPARAM wParam, LPARAM lParam);

public:
	static void RunMessageLoop();

	MessageWindow(const std::wstring &className, const std::wstring &windowName, const HINSTANCE &hInstance = GetModuleHandle(NULL), const Window &parent = Window::NullWindow, const wchar_t *iconResource = MAKEINTRESOURCE(MAINICON));
	using CALLBACKCOOKIE = unsigned long long;
	CALLBACKCOOKIE RegisterCallback(unsigned int message, const callback_t &callback);
	inline CALLBACKCOOKIE RegisterCallback(const std::wstring &message, const callback_t &callback)
	{
		return RegisterCallback(RegisterWindowMessage(message.c_str()), callback);
	}
	void UnregisterCallback(CALLBACKCOOKIE cookie);
	~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;
};