#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

#include "window.hpp"
#include "windowclass.hpp"

#include "resource.h"

class MessageWindow : public Window {

protected:
	typedef std::function<long(WPARAM, LPARAM)> m_CallbackFunction;

private:
	std::unordered_map<unsigned int, std::vector<std::pair<unsigned short, m_CallbackFunction>>> m_CallbackMap;
	WindowClass m_WindowClass;

	LRESULT WindowProcedure(const Window &window, unsigned int uMsg, WPARAM wParam, LPARAM lParam);

public:
	MessageWindow(const std::wstring &className, const std::wstring &windowName, const HINSTANCE &hInstance = GetModuleHandle(NULL), const wchar_t *iconResource = MAKEINTRESOURCE(MAINICON));
	typedef unsigned long long CALLBACKCOOKIE;
	CALLBACKCOOKIE RegisterCallback(unsigned int message, const m_CallbackFunction &callback);
	inline CALLBACKCOOKIE RegisterCallback(const std::wstring &message, const m_CallbackFunction &callback)
	{
		return RegisterCallback(RegisterWindowMessage(message.c_str()), callback);
	}
	bool UnregisterCallback(CALLBACKCOOKIE cookie);
	~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;
};