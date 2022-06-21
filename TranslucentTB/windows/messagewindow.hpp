#pragma once
#include "arch.h"
#include <member_thunk/page.hpp>
#include <memory>
#include <windef.h>

#include "../resources/ids.h"
#include "util/maybe_delete.hpp"
#include "util/null_terminated_string_view.hpp"
#include "window.hpp"
#include "windowclass.hpp"

class MessageWindow : public Window {
private:
	std::unique_ptr<WindowClass, Util::maybe_delete> m_WindowClass;
	const wchar_t *m_IconResource;

	member_thunk::page m_ProcPage;

	void init(Util::null_terminated_wstring_view windowName, DWORD style, DWORD extended_style, Window parent);

	inline static const wchar_t *const DEFAULT_ICON = MAKEINTRESOURCE(IDI_MAINICON);

protected:
	inline HINSTANCE hinstance() const noexcept
	{
		return m_WindowClass->hinstance();
	}

	inline virtual LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DPICHANGED:
			m_WindowClass->ChangeIcon(m_WindowHandle, m_IconResource);
			break;
		}

		return DefWindowProc(m_WindowHandle, uMsg, wParam, lParam);
	}

	MessageWindow(WindowClass &classRef, Util::null_terminated_wstring_view windowName, DWORD style = 0, DWORD extended_style = 0, Window parent = Window::NullWindow, const wchar_t *iconResource = DEFAULT_ICON);
	MessageWindow(Util::null_terminated_wstring_view className, Util::null_terminated_wstring_view windowName, HINSTANCE hInstance, DWORD style = 0, DWORD extended_style = 0, Window parent = Window::NullWindow, const wchar_t *iconResource = DEFAULT_ICON);
	~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;

public:
	inline static WindowClass MakeWindowClass(Util::null_terminated_wstring_view className, HINSTANCE hInstance, const wchar_t *iconResource = DEFAULT_ICON)
	{
		return { DefWindowProc, className, iconResource, hInstance };
	}
};
