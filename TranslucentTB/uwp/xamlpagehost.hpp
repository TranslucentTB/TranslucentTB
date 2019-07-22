#pragma once
#include "../windows/messagewindow.hpp"

#include "arch.h"
#include <ShellScalingApi.h>
#include <string>
#include <type_traits>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/TranslucentTB.Pages.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <windowsx.h>

namespace xaml = winrt::Windows::UI::Xaml;

enum class CenteringStrategy {
	Mouse,
	Monitor
};

template<typename T>
class XamlPageHost : public MessageWindow {
private:
	CenteringStrategy m_CenteringStrategy;
	Window m_interopWnd;
	FILTERCOOKIE m_FilterCookie;

	xaml::Hosting::DesktopWindowXamlSource m_source;
	T m_content;
	winrt::event_token m_TitleChangedToken;

	inline static float GetDpiScale(HMONITOR mon)
	{
		UINT dpiX, dpiY;
		winrt::check_hresult(GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY));

		return dpiX / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
	}

	inline static RECT CenterRectOnMouse(LONG width, LONG height, const RECT &workArea, POINT mouse)
	{
		RECT temp;
		temp.left = mouse.x - (width / 2);
		temp.right = temp.left + width;

		temp.top = mouse.y - (height / 2);
		temp.bottom = temp.top + height;

		if (win32::RectFitsInRect(workArea, temp))
		{
			return temp;
		}

		const bool rightDoesntFits = temp.right > workArea.right;
		const bool leftDoesntFits = temp.left < workArea.left;
		const bool bottomDoesntFits = temp.bottom > workArea.bottom;
		const bool topDoesntFits = temp.top < workArea.top;

		if ((rightDoesntFits && leftDoesntFits) || (bottomDoesntFits && topDoesntFits))
		{
			// Doesn't fits in the monitor work area (lol wat)	
			throw winrt::hresult_out_of_bounds();
		}

		// Offset the rect so that it is completely in the work area	
		int x_offset = 0;
		if (rightDoesntFits)
		{
			x_offset = workArea.right - temp.right; // Negative offset	
		}
		else if (leftDoesntFits)
		{
			x_offset = workArea.left - temp.left;
		}

		int y_offset = 0;
		if (bottomDoesntFits)
		{
			y_offset = workArea.bottom - temp.bottom; // Negative offset	
		}
		else if (topDoesntFits)
		{
			y_offset = workArea.top - temp.top;
		}

		if (!OffsetRect(&temp, x_offset, y_offset))
		{
			throw winrt::hresult_error(E_FAIL);
		}

		return temp;
	}

	inline static RECT CenterRectOnMonitor(LONG width, LONG height, const RECT &workArea)
	{
		const auto workWidth = workArea.right - workArea.left;
		const auto workHeight = workArea.bottom - workArea.top;

		RECT temp;
		temp.left = (workWidth - width) / 2;
		temp.right = temp.left + width;

		temp.top = (workHeight - height) / 2;
		temp.bottom = temp.top + height;

		if (!OffsetRect(&temp, workArea.left, workArea.top))
		{
			throw winrt::hresult_error(E_FAIL);
		}

		return temp;
	}

	inline void SetTitle(...)
	{
		SetWindowText(m_WindowHandle, m_content.Title().c_str());
	}

	inline void RegisterCallbacks()
	{
		RegisterCallback(WM_NCCALCSIZE, [](...)
		{
			return 0;
		});

		RegisterCallback(WM_SIZE, [this](WPARAM, LPARAM lParam)
		{
			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);
			winrt::check_bool(SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, x, y, 0));

			const float scale = GetDpiScale(monitor());
			m_content.Arrange(xaml::RectHelper::FromCoordinatesAndDimensions(0, 0, x / scale, y / scale));
			return 0;
		});

		RegisterCallback(WM_DPICHANGED, [this](WPARAM, LPARAM lParam)
		{
			PositionWindow(*reinterpret_cast<RECT *>(lParam));
			return 0;
		});
	}

	inline RECT CenterWindow(LONG width, LONG height, HMONITOR mon, POINT mouse)
	{
		MONITORINFO mi = { sizeof(mi) };
		if (!GetMonitorInfo(mon, &mi))
		{
			throw winrt::hresult_error(E_FAIL);
		}

		if (m_CenteringStrategy == CenteringStrategy::Mouse)
		{
			return CenterRectOnMouse(width, height, mi.rcWork, mouse);
		}
		else
		{
			return CenterRectOnMonitor(width, height, mi.rcWork);
		}
	}

	RECT CalculateWindowPosition()
	{
		m_content.Measure(xaml::SizeHelper::FromDimensions(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));

		winrt::Windows::Foundation::Size size = m_content.DesiredSize();

		POINT point;
		winrt::check_bool(GetCursorPos(&point));
		const HMONITOR mon = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);

		const float scale = GetDpiScale(mon);
		size.Height *= scale;
		size.Width *= scale;

		return CenterWindow(size.Width, size.Height, mon, point);
	}

	void PositionWindow(const RECT &rect, bool showWindow = false)
	{
		winrt::check_bool(SetWindowPos(m_WindowHandle, Window::NullWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW | SWP_FRAMECHANGED : 0));
		winrt::check_bool(SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW : 0));
	}

public:
	// TODO: support multiple instances
	// TODO: better class name lol
	// todo: movable window
	template<typename... Args>
	inline XamlPageHost(HINSTANCE hInst, CenteringStrategy strategy, Args&&... args) :
		MessageWindow(L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", { }, hInst, WS_OVERLAPPED),
		m_CenteringStrategy(strategy),
		m_interopWnd(Window::NullWindow),
		m_FilterCookie(0),
		// Don't construct the XAML stuff already.
		m_content(nullptr)
	{
		auto nativeSource = m_source.as<IDesktopWindowXamlSourceNative2>();
		winrt::check_hresult(nativeSource->AttachToWindow(m_WindowHandle));

		winrt::check_hresult(nativeSource->get_WindowHandle(m_interopWnd.put()));

		m_FilterCookie = RegisterThreadMessageFilter([nativeSource](const MSG &msg)
		{
			BOOL result;
			winrt::check_hresult(nativeSource->PreTranslateMessage(&msg, &result));
			return result;
		});

		m_content = T(std::forward<Args>(args)...);

		// Make sure T is a frameless page
		WINRT_ASSERT(m_content.try_as<winrt::TranslucentTB::Pages::FramelessPage>());

		m_source.Content(m_content);

		SetTitle();
		m_TitleChangedToken.value = m_content.RegisterPropertyChangedCallback(winrt::TranslucentTB::Pages::FramelessPage::TitleProperty(), { this, &XamlPageHost::SetTitle });

		// Magic that gives us shadows
		const MARGINS margins = { 1 };
		DwmExtendFrameIntoClientArea(m_WindowHandle, &margins);

		RegisterCallbacks();

		PositionWindow(CalculateWindowPosition(), true);

		UpdateWindow(m_WindowHandle);
		SetFocus(m_WindowHandle);
	}

	inline T *operator ->()
	{
		return &m_content;
	}

	inline ~XamlPageHost()
	{
		m_content.UnregisterPropertyChangedCallback(winrt::TranslucentTB::Pages::FramelessPage::TitleProperty(), std::exchange(m_TitleChangedToken.value, 0));
		m_content = nullptr;

		UnregisterThreadMessageFilter(std::exchange(m_FilterCookie, 0));

		m_source.Close();
		m_source = nullptr;
	}

	XamlPageHost(const XamlPageHost &) = delete;
	XamlPageHost &operator =(const XamlPageHost &) = delete;
};