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
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_source;
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource::TakeFocusRequested_revoker m_focusRevoker;

protected:
	static float GetDpiScale(HMONITOR mon);

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	RECT CalculateWindowPosition(winrt::Windows::Foundation::Size size);
	void PositionWindow(const RECT &rect, bool showWindow = false);
	void PositionInteropWindow(int x, int y);

	BaseXamlPageHost(Util::null_terminated_wstring_view className, HINSTANCE hInst);

public:
	inline constexpr winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource &source() noexcept
	{
		return m_source;
	}

	inline virtual ~BaseXamlPageHost()
	{
		m_focusRevoker.revoke();
		m_source.Close();
		m_source = nullptr;
	}

	BaseXamlPageHost(const BaseXamlPageHost&) = delete;
	BaseXamlPageHost& operator =(const BaseXamlPageHost&) = delete;
};
