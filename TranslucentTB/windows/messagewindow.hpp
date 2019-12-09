#pragma once
#include "arch.h"
#include <member_thunk/common.hpp>
#include <windef.h>
#include <wil/resource.h>

#include "../resources/ids.h"
#include "undoc/uxtheme.hpp"
#include "util/null_terminated_string_view.hpp"
#include "window.hpp"
#include "windowclass.hpp"

class MessageWindow : public Window {
protected:
	static const wil::unique_hmodule uxtheme;

private:
	static const PFN_ALLOW_DARK_MODE_FOR_WINDOW AllowDarkModeForWindow;
	static void NTAPI DeleteThisAPC(ULONG_PTR that);

	// Order important: the thunk needs to be destroyed after the window class
	std::unique_ptr<member_thunk::thunk<WNDPROC>> m_ProcedureThunk;
	WindowClass m_WindowClass;
	const wchar_t *m_IconResource;

	void Destroy();

protected:
	static const PFN_SHOULD_SYSTEM_USE_DARK_MODE ShouldSystemUseDarkMode;
	void HeapDeletePostNcDestroy();

	inline HINSTANCE hinstance() const noexcept
	{
		return m_WindowClass.hinstance();
	}

	inline static bool DarkModeAvailable() noexcept
	{
		return uxtheme && AllowDarkModeForWindow && ShouldSystemUseDarkMode;
	}

	inline virtual LRESULT CALLBACK MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	virtual ~MessageWindow();

	inline MessageWindow(const MessageWindow &) = delete;
	inline MessageWindow &operator =(const MessageWindow &) = delete;
};
