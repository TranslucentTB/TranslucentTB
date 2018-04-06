#include "messagewindow.hpp"
#include <algorithm>
#include <random>

long MessageWindow::m_StaticCallback(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam)
{
	MessageWindow *pThis;

	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT lpcs = *reinterpret_cast<CREATESTRUCT *>(lParam);
		pThis = static_cast<MessageWindow *>(lpcs.lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<long>(pThis));
	}
	else
	{
		pThis = reinterpret_cast<MessageWindow *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	if (pThis)
	{
		MessageWindow &window = *pThis;
		const auto &callbackVector = window.m_CallbackMap[uMsg];
		if (callbackVector.size() > 0)
		{
			long result;
			for (const auto &callbackPair : callbackVector)
			{
				result = (std::max)(callbackPair.second(window, wParam, lParam), result);
			}
			return result;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

MessageWindow::MessageWindow(const std::wstring &className, const std::wstring &windowName, const HINSTANCE &hInstance, const wchar_t *iconResource) :
	m_WindowClass(m_StaticCallback, className, iconResource, 0, hInstance)
{
	m_WindowHandle = Window::Create(0, className, windowName, 0, 0, 0, 0, 0, nullptr, 0, hInstance, this);
}

MessageWindow::CALLBACKCOOKIE MessageWindow::RegisterCallback(unsigned int message, const m_CallbackFunction &callback)
{
	std::random_device seed;
	std::mt19937 rng(seed());
	std::uniform_int_distribution<unsigned short> ushort_values(0, USHRT_MAX);

	unsigned short secret = ushort_values(rng);
	m_CallbackMap[message].push_back(std::make_pair(secret, callback));

	return (static_cast<CALLBACKCOOKIE>(secret) << 32) + message;
}

MessageWindow::CALLBACKCOOKIE MessageWindow::RegisterCallback(const std::wstring &message, const m_CallbackFunction &callback)
{
	return RegisterCallback(RegisterWindowMessage(message.c_str()), callback);
}

bool MessageWindow::UnregisterCallback(CALLBACKCOOKIE cookie)
{
	unsigned int message = cookie & 0xFFFFFFFF;
	unsigned short secret = (cookie >> 32) & 0xFFFF;

	auto &callbackVector = m_CallbackMap[message];
	for (auto &callbackPair : callbackVector)
	{
		if (callbackPair.first == secret)
		{
			std::swap(callbackPair, callbackVector.back());
			callbackVector.pop_back();
			return true;
		}
	}

	return false;
}

MessageWindow::~MessageWindow()
{
	DestroyWindow(m_WindowHandle);
}
