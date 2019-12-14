#include "messagewindow.hpp"
#include <member_thunk/member_thunk.hpp>

#include "../mainappwindow.hpp"
#include "../../ProgramLog/error.hpp"

// TODO: use function local statics
const wil::unique_hmodule MessageWindow::uxtheme(LoadLibraryEx(UXTHEME_DLL, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));

const PFN_ALLOW_DARK_MODE_FOR_WINDOW MessageWindow::AllowDarkModeForWindow =
	reinterpret_cast<PFN_ALLOW_DARK_MODE_FOR_WINDOW>(GetProcAddress(uxtheme.get(), ADMFW_ORDINAL));

const PFN_SHOULD_SYSTEM_USE_DARK_MODE MessageWindow::ShouldSystemUseDarkMode =
	reinterpret_cast<PFN_SHOULD_SYSTEM_USE_DARK_MODE>(GetProcAddress(uxtheme.get(), SSUDM_ORDINAL));

// Static initialization order reeee
const PFN_SET_PREFERRED_APP_MODE MainAppWindow::SetPreferredAppMode =
	reinterpret_cast<PFN_SET_PREFERRED_APP_MODE>(GetProcAddress(MessageWindow::uxtheme.get(), SPAM_ORDINAL));

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
	LONG_PTR val = SetWindowLongPtr(m_WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_ProcedureThunk->get_thunked_function()));
	if (!val && GetLastError() != NO_ERROR)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to update window procedure!");
	}

	if (DarkModeAvailable())
	{
		AllowDarkModeForWindow(m_WindowHandle, true);
	}
}

MessageWindow::~MessageWindow()
{
	Destroy();
}
