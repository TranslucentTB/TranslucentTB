#pragma once
#include "../windows/messagewindow.hpp"

#include "arch.h"
#include <windef.h>

#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"


class BaseXamlPageHost : public MessageWindow {
private:
	Window m_interopWnd;
	winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_manager;
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_source;
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource::TakeFocusRequested_revoker m_focusRevoker;

	bool PreTranslateMessage(const MSG &msg) override;

protected:
	static float GetDpiScale(HMONITOR mon);

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	RECT CalculateWindowPosition(winrt::Windows::Foundation::Size size);
	void PositionWindow(const RECT &rect, bool showWindow = false);
	void PositionInteropWindow(int x, int y);

	BaseXamlPageHost(Util::null_terminated_wstring_view className, HINSTANCE hInst);

	inline constexpr winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource &source() noexcept
	{
		return m_source;
	}

public:
	inline ~BaseXamlPageHost()
	{
		m_focusRevoker.revoke();
		m_source.Close();
		m_source = nullptr;
		m_manager.Close();
		m_manager = nullptr;

		winrt::uninit_apartment();
	}

	BaseXamlPageHost(const BaseXamlPageHost&) = delete;
	BaseXamlPageHost& operator =(const BaseXamlPageHost&) = delete;
};
