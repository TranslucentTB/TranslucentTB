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

	static float GetDpiScale(HMONITOR mon);

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

protected:
	RECT CalculateWindowPosition(winrt::Windows::Foundation::Size size);
	void PositionWindow(const RECT &rect, bool showWindow = false);
	virtual void ArrangeContent(winrt::Windows::Foundation::Rect area) = 0;

	BaseXamlPageHost(Util::null_terminated_wstring_view className, HINSTANCE hInst);

public:
	inline constexpr winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource &source() noexcept
	{
		return m_source;
	}

	inline virtual ~BaseXamlPageHost()
	{
		m_source.Close();
		m_source = nullptr;
	}

	BaseXamlPageHost(const BaseXamlPageHost&) = delete;
	BaseXamlPageHost& operator =(const BaseXamlPageHost&) = delete;
};
