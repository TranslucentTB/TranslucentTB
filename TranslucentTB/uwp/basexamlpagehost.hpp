#pragma once
#include "../windows/messagewindow.hpp"

#include "arch.h"
#include <windef.h>

#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>
#include "redefgetcurrenttime.h"

#include "../windows/windowclass.hpp"

enum class xaml_startup_position {
	center,
	mouse
};

class BaseXamlPageHost : public MessageWindow {
private:
	Window m_interopWnd;
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_source;
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource::TakeFocusRequested_revoker m_focusRevoker;

	void UpdateFrame();

protected:
	static HMONITOR GetInitialMonitor(POINT &cursor, xaml_startup_position position);
	static float GetDpiScale(HMONITOR mon);
	static void CalculateInitialPosition(int &x, int &y, int width, int height, POINT cursor, const RECT &workArea, xaml_startup_position position) noexcept;
	static bool AdjustWindowPosition(int &x, int &y, int width, int height, const RECT &workArea) noexcept;

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void ResizeWindow(int x, int y, int width, int height, bool move, UINT flags = 0);
	void Flash() noexcept;
	BaseXamlPageHost(WindowClass &classRef);

	inline void Cleanup() noexcept
	{
		m_focusRevoker.revoke();
	}

	inline void BeforeDelete()
	{
		m_source.Close();
		m_source = nullptr;
	}

public:
	virtual ~BaseXamlPageHost() = default;

	constexpr winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource &source() noexcept
	{
		return m_source;
	}

	virtual winrt::TranslucentTB::Xaml::Pages::FramelessPage page() noexcept = 0;
};
