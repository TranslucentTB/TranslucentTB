#pragma once
#include <functional>
#include <unordered_map>

#include "../resources/ids.h"
#include "util/random.hpp"
#include "window.hpp"
#include "windowclass.hpp"

class MessageWindow : public Window {
protected:
	using callback_t = std::function<long(WPARAM, LPARAM)>;

private:
	std::unordered_map<unsigned int, std::unordered_map<unsigned short, callback_t>> m_CallbackMap;
	WindowClass m_WindowClass;

	LRESULT WindowProcedure(Window window, unsigned int uMsg, WPARAM wParam, LPARAM lParam);

public:
	static WPARAM RunMessageLoop();

	MessageWindow(const std::wstring &className, const std::wstring &windowName, HINSTANCE hInstance = GetModuleHandle(nullptr), Window parent = Window::NullWindow, const wchar_t *iconResource = MAKEINTRESOURCE(IDI_MAINICON));
	using CALLBACKCOOKIE = unsigned long long;

	inline CALLBACKCOOKIE RegisterCallback(unsigned int message, callback_t callback)
	{
		auto &callbackMap = m_CallbackMap[message];

		const auto secret = Util::GetSecret(callbackMap);

		callbackMap[secret] = std::move(callback);

		return (static_cast<CALLBACKCOOKIE>(secret) << 32) + message;
	}

	inline CALLBACKCOOKIE RegisterCallback(const std::wstring &message, callback_t callback)
	{
		return RegisterCallback(RegisterWindowMessage(message.c_str()), std::move(callback));
	}

	inline void UnregisterCallback(CALLBACKCOOKIE cookie)
	{
		unsigned int message = cookie & 0xFFFFFFFF;
		unsigned short secret = (cookie >> 32) & 0xFFFF;

		m_CallbackMap[message].erase(secret);
	}

	~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;
};