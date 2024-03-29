#include "messagewindow.hpp"
#include <member_thunk/member_thunk.hpp>

#include "../../ProgramLog/error/win32.hpp"
#include "../../ProgramLog/error/std.hpp"

void MessageWindow::init(Util::null_terminated_wstring_view windowName, DWORD style, DWORD extended_style, Window parent)
{
	m_WindowHandle = Window::Create(extended_style, *m_WindowClass, windowName, style, 0, 0, 0, 0, parent);
	if (!m_WindowHandle)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to create message window!");
	}

	const auto proc = m_ProcPage.make_thunk<WNDPROC>(this, &MessageWindow::MessageHandler);
	m_ProcPage.mark_executable();

	set_long_ptr<spdlog::level::critical>(GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
}

MessageWindow::MessageWindow(WindowClass &classRef, Util::null_terminated_wstring_view windowName, DWORD style, DWORD extended_style, Window parent, const wchar_t *iconResource) :
	m_WindowClass(&classRef, false),
	m_IconResource(iconResource),
	m_ProcPage(member_thunk::allocate_page())
{
	init(windowName, style, extended_style, parent);
}

MessageWindow::MessageWindow(Util::null_terminated_wstring_view className, Util::null_terminated_wstring_view windowName, HINSTANCE hInstance, DWORD style, DWORD extended_style, Window parent, const wchar_t *iconResource) :
	m_WindowClass(new WindowClass(MakeWindowClass(className, hInstance, iconResource)), true),
	m_IconResource(iconResource),
	m_ProcPage(member_thunk::allocate_page())
{
	init(windowName, style, extended_style, parent);
}

MessageWindow::~MessageWindow()
{
	if (!DestroyWindow(m_WindowHandle))
	{
		LastErrorHandle(spdlog::level::info, L"Failed to destroy message window!");
	}
}
