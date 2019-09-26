#include "messagewindow.hpp"
#include <algorithm>

#include "../../ProgramLog/error.hpp"

thread_local std::unordered_map<unsigned int, MessageWindow::filter_t> MessageWindow::s_FilterMap;

LRESULT MessageWindow::WindowProcedure(Window window, unsigned int uMsg, WPARAM wParam, LPARAM lParam)
{
	if (const auto iter = m_CallbackMap.find(uMsg); iter != m_CallbackMap.end() && !iter->second.empty())
	{
		long result = 0;
		for (const auto &[_, callback] : iter->second)
		{
			result = std::max(callback(wParam, lParam), result);
		}
		return result;
	}

	return DefWindowProc(window, uMsg, wParam, lParam);
}

WPARAM MessageWindow::RunMessageLoop()
{
	MSG msg;
	BOOL ret;
	while ((ret = GetMessage(&msg, nullptr, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			if (!s_FilterMap.empty())
			{
				for (const auto &[_, filter] : s_FilterMap)
				{
					if (filter(msg))
					{
						continue;
					}
				}
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			LastErrorHandle(spdlog::level::critical, L"GetMessage failed!");
		}
	}

	return msg.wParam;
}

MessageWindow::MessageWindow(const std::wstring &className, const std::wstring &windowName, HINSTANCE hInstance, unsigned long style, Window parent, const wchar_t *iconResource) :
	m_WindowClass(
		std::bind(&MessageWindow::WindowProcedure, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
		className,
		iconResource,
		0,
		hInstance
	)
{
	m_WindowHandle = Window::Create(0, m_WindowClass.atom(), windowName, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, 0, hInstance, this);

	if (!m_WindowHandle)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to create message window!");
	}

	RegisterCallback(WM_DPICHANGED, [this, iconResource](...)
	{
		m_WindowClass.ChangeIcon(*this, iconResource);

		return 0;
	});
}

MessageWindow::~MessageWindow()
{
	if (!DestroyWindow(m_WindowHandle))
	{
		LastErrorHandle(spdlog::level::info, L"Failed to destroy message window!");
	}
}
