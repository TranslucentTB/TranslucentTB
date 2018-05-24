#include "messagewindow.hpp"
#include <algorithm>

#include "ttberror.hpp"
#include "util.hpp"

long MessageWindow::WindowProcedure(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam)
{
	MessageWindow *pThis;

	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT lpcs = *reinterpret_cast<CREATESTRUCT *>(lParam);
		pThis = static_cast<MessageWindow *>(lpcs.lpCreateParams);

		pThis->set_ptr(hWnd);
	}
	else
	{
		pThis = get_ptr(hWnd);
	}

	if (pThis)
	{
		MessageWindow &window = *pThis;
		const auto &callbackVector = window.m_CallbackMap[uMsg];
		if (callbackVector.size() > 0)
		{
			long result = 0;
			for (const auto &callbackPair : callbackVector)
			{
				result = (std::max)(callbackPair.second(window, wParam, lParam), result);
			}
			return result;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void MessageWindow::set_ptr(const HWND &hwnd)
{
	SetLastError(0);
	if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this)) && GetLastError() != 0)
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to set window pointer!");
	}
}

MessageWindow *MessageWindow::get_ptr(const HWND &hwnd)
{
	SetLastError(0);
	MessageWindow *window = reinterpret_cast<MessageWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (!window && GetLastError() != 0)
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to get window pointer!");
	}

	return window;
}

MessageWindow::MessageWindow(const std::wstring &className, const std::wstring &windowName, const HINSTANCE &hInstance, const wchar_t *iconResource) :
	m_WindowClass(WindowProcedure, className, iconResource, 0, hInstance)
{
	m_WindowHandle = Window::Create(0, m_WindowClass, windowName, 0, 0, 0, 0, 0, Window::NullWindow, 0, hInstance, this);

	if (!m_WindowHandle)
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to create message window!");
	}
}

MessageWindow::CALLBACKCOOKIE MessageWindow::RegisterCallback(unsigned int message, const m_CallbackFunction &callback)
{
	unsigned short secret = Util::GetRandomNumber<unsigned short>();
	m_CallbackMap[message].push_back(std::make_pair(secret, callback));

	return (static_cast<CALLBACKCOOKIE>(secret) << 32) + message;
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
	if (!DestroyWindow(m_WindowHandle))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy message window!");
	}
}