#include "messagewindow.hpp"
#include <member_thunk/member_thunk.hpp>

#include "undoc/dynamicloader.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "../../ProgramLog/error/std.hpp"

void MessageWindow::DeleteThisAPC(ULONG_PTR that)
{
	delete reinterpret_cast<MessageWindow *>(that);
}

void MessageWindow::Destroy()
{
	if (m_WindowHandle != Window::NullWindow)
	{
		if (!DestroyWindow(m_WindowHandle))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to destroy message window!");
		}
	}
}

void MessageWindow::HeapDeletePostNcDestroy()
{
	m_WindowHandle = Window::NullWindow;

	if (!QueueUserAPC(DeleteThisAPC, GetCurrentThread(), reinterpret_cast<ULONG_PTR>(this)))
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to queue class deletion, memory will leak.");
	}
}

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
		m_ProcedureThunk = member_thunk::make(this, &MessageWindow::MessageHandler);
	}
	StdSystemErrorCatch(spdlog::level::critical, L"Failed to create window member thunk");

	SetLastError(NO_ERROR);
	const LONG_PTR val = SetWindowLongPtr(m_WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_ProcedureThunk->get_thunked_function()));
	if (!val)
	{
		if (const DWORD lastErr = GetLastError(); lastErr != NO_ERROR)
		{
			HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::critical, L"Failed to update window procedure!");
		}
	}

	if (DynamicLoader::uxtheme())
	{
		if (const auto admfm = DynamicLoader::AllowDarkModeForWindow())
		{
			admfm(m_WindowHandle, true);
		}
	}
}

MessageWindow::~MessageWindow()
{
	Destroy();
}
