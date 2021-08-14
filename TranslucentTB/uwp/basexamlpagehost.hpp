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
#include "xamldragregion.hpp"

enum class xaml_startup_position {
	center,
	mouse
};

class BaseXamlPageHost : public MessageWindow {
private:
	XamlDragRegion m_DragRegion;
	Window m_interopWnd;
	wuxh::DesktopWindowXamlSource m_source;
	winrt::event_token m_focusToken;
	wil::unique_hbrush m_BackgroundBrush;
	winrt::Windows::UI::Color m_BackgroundColor = { };

	void UpdateFrame();

protected:
	static wf::Rect ScaleRect(wf::Rect rect, float scale);
	static HMONITOR GetInitialMonitor(POINT &cursor, xaml_startup_position position);
	static float GetDpiScale(HMONITOR mon);
	static void CalculateInitialPosition(int &x, int &y, int width, int height, POINT cursor, const RECT &workArea, xaml_startup_position position) noexcept;
	static bool AdjustWindowPosition(int &x, int &y, int width, int height, const RECT &workArea) noexcept;

	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void ResizeWindow(int x, int y, int width, int height, bool move, UINT flags = 0);
	void PositionDragRegion(wf::Rect position, wf::Rect buttonsRegion, UINT flags = 0);
	void Flash() noexcept;
	bool PaintBackground(HDC dc, const RECT &target, winrt::Windows::UI::Color col);
	BaseXamlPageHost(WindowClass &classRef, WindowClass &dragRegionClass);

	void Cleanup()
	{
		if (m_source)
		{
			if (m_focusToken)
			{
				m_source.TakeFocusRequested(m_focusToken);
				m_focusToken.value = 0;
			}
			
			m_source.Close();
			m_source = nullptr;
		}

		m_BackgroundBrush.reset();
	}

public:
	virtual ~BaseXamlPageHost()
	{
		Cleanup();
	}

	constexpr const wuxh::DesktopWindowXamlSource &source() noexcept
	{
		return m_source;
	}

	virtual winrt::TranslucentTB::Xaml::Pages::FramelessPage page() noexcept = 0;
	virtual bool TryClose() = 0;
};
