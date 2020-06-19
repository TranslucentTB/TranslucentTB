#pragma once
#include "arch.h"
#include <member_thunk/common.hpp>
#include <windef.h>

#include "../resources/ids.h"
#include "util/null_terminated_string_view.hpp"
#include "window.hpp"
#include "windowclass.hpp"

class MessageWindow : public Window {
private:
	WindowClass m_WindowClass;
	const wchar_t *m_IconResource;

	std::unique_ptr<member_thunk::thunk<WNDPROC>> m_ProcThunk;

protected:
	inline HINSTANCE hinstance() const noexcept
	{
		return m_WindowClass.hinstance();
	}

	inline virtual bool PreTranslateMessage(const MSG &)
	{
		return false;
	}

	inline virtual LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DPICHANGED:
			m_WindowClass.ChangeIcon(m_WindowHandle, m_IconResource);
			break;
		}

		return DefWindowProc(m_WindowHandle, uMsg, wParam, lParam);
	}

	MessageWindow(Util::null_terminated_wstring_view className, Util::null_terminated_wstring_view windowName, HINSTANCE hInstance, unsigned long style = 0, Window parent = Window::NullWindow, const wchar_t *iconResource = MAKEINTRESOURCE(IDI_MAINICON));

public:
	WPARAM Run();

	~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;
};
