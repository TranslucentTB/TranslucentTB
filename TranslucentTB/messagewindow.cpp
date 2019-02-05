#include "messagewindow.hpp"
#include <algorithm>

#include "ttberror.hpp"
#include "util.hpp"

LRESULT MessageWindow::WindowProcedure(Window window, unsigned int uMsg, WPARAM wParam, LPARAM lParam)
{
	const auto &callbackList = m_CallbackMap[uMsg];
	if (!callbackList.empty())
	{
		long result = 0;
		for (const auto &[_, callback] : callbackList)
		{
			result = (std::max)(callback(wParam, lParam), result);
		}
		return result;
	}

	return DefWindowProc(window, uMsg, wParam, lParam);
}

void MessageWindow::RunMessageLoop()
{
	MSG msg;
	BOOL ret;
	while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			LastErrorHandle(Error::Level::Fatal, L"GetMessage failed!");
		}
	}
}

MessageWindow::MessageWindow(const std::wstring &className, const std::wstring &windowName, HINSTANCE hInstance, Window parent, const wchar_t *iconResource) :
	m_WindowClass(
		std::bind(&MessageWindow::WindowProcedure, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
		className,
		iconResource,
		0,
		hInstance
	)
{
	m_WindowHandle = Window::Create(0, m_WindowClass, windowName, 0, 0, 0, 0, 0, parent, 0, hInstance, this);

	if (!m_WindowHandle)
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to create message window!");
	}

	RegisterCallback(WM_DPICHANGED, [this, iconResource](...)
	{
		m_WindowClass.ChangeIcon(*this, iconResource);

		return 0;
	});
}

MessageWindow::CALLBACKCOOKIE MessageWindow::RegisterCallback(unsigned int message, callback_t callback)
{
	unsigned short secret = Util::GetRandomNumber<unsigned short>();
	m_CallbackMap[message].push_front({ secret, std::move(callback) });

	return (static_cast<CALLBACKCOOKIE>(secret) << 32) + message;
}

void MessageWindow::UnregisterCallback(CALLBACKCOOKIE cookie)
{
	unsigned int message = cookie & 0xFFFFFFFF;
	unsigned short secret = (cookie >> 32) & 0xFFFF;

	m_CallbackMap[message].remove_if([&secret](const std::pair<unsigned short, callback_t> &pair) -> bool
	{
		return pair.first == secret;
	});
}

MessageWindow::~MessageWindow()
{
	if (!DestroyWindow(m_WindowHandle))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy message window!");
	}
}