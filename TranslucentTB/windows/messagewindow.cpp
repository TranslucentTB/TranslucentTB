#include "messagewindow.hpp"
#include <member_thunk/member_thunk.hpp>

#include "undoc/dynamicloader.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "../../ProgramLog/error/std.hpp"

MessageWindow::MessageWindow(Util::null_terminated_wstring_view className, Util::null_terminated_wstring_view windowName, HINSTANCE hInstance, unsigned long style, Window parent, const wchar_t *iconResource) :
	m_WindowClass(DefWindowProc, className, iconResource, hInstance),
	m_IconResource(iconResource)
{
	m_WindowHandle = Window::Create(0, m_WindowClass, windowName, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent);
	if (!m_WindowHandle)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to create message window!");
	}

	try
	{
		m_ProcThunk = member_thunk::make(this, &MessageWindow::MessageHandler);
	}
	StdSystemErrorCatch(spdlog::level::critical, L"Failed to create message handler thunk!");

	SetLastError(NO_ERROR);
	if (!SetWindowLongPtr(m_WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_ProcThunk->get_thunked_function())))
	{
		LastErrorVerify(spdlog::level::critical, L"Failed to update window procedure!");
	}

	if (DynamicLoader::uxtheme())
	{
		if (const auto admfm = DynamicLoader::AllowDarkModeForWindow())
		{
			admfm(m_WindowHandle, true);
		}
	}
}

WPARAM MessageWindow::Run()
{
	while (true)
	{
		switch (MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
		case WAIT_OBJECT_0:
			for (MSG msg; PeekMessage(&msg, 0, 0, 0, PM_REMOVE);)
			{
				if (msg.message != WM_QUIT)
				{
					if (!PreTranslateMessage(msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				else
				{
					return msg.wParam;
				}
			}
			[[fallthrough]];
		case WAIT_IO_COMPLETION:
			continue;

		case WAIT_FAILED:
			LastErrorHandle(spdlog::level::critical, L"Failed to enter alertable wait state!");

		default:
			MessagePrint(spdlog::level::critical, L"MsgWaitForMultipleObjectsEx returned an unexpected value!");
		}
	}
}

MessageWindow::~MessageWindow()
{
	if (!DestroyWindow(m_WindowHandle))
	{
		LastErrorHandle(spdlog::level::info, L"Failed to destroy message window!");
	}
}
