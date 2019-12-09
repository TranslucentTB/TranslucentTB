#pragma once
#include "../windows/messagewindow.hpp"

#include "arch.h"
#include <ShellScalingApi.h>
#include <string>
#include <type_traits>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <windowsx.h>

#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>
#include "redefgetcurrenttime.h"

#include "util/strings.hpp"
#include "win32.hpp"

enum class CenteringStrategy {
	Mouse,
	Monitor
};

// TODO: base xaml page host to avoid lots of template duplication
template<typename T>
class XamlPageHost : public MessageWindow {
private:
	Window m_interopWnd;

	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_source;
	T m_content;
	winrt::event_token m_TitleChangedToken;

	static constexpr std::wstring_view ExtractTypename()
	{
		constexpr std::wstring_view prefix = L"XamlPageHost<struct winrt::";
		constexpr std::wstring_view suffix = L">::ExtractTypename";

		std::wstring_view funcsig = UTIL_WIDEN(__FUNCSIG__);
		funcsig.remove_prefix(funcsig.find(prefix) + prefix.length());
		funcsig.remove_suffix(funcsig.length() - funcsig.rfind(suffix));

		return funcsig;
	}

	inline static const std::wstring &GetClassName()
	{
		static constexpr std::wstring_view type_name = ExtractTypename();
		static const std::wstring class_name(type_name);

		return class_name;
	}

	inline static UINT GetDpi(HMONITOR mon)
	{
		UINT dpiX, dpiY;
		winrt::check_hresult(GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY));

		return dpiX;
	}

	inline static float GetDpiScale(HMONITOR mon)
	{
		return GetDpi(mon) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
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
			// TODO: throwing might not be a good idea here
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

	inline LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_NCCALCSIZE:
			return 0;

		case WM_SIZE:
		{
			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);
			winrt::check_bool(SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, x, y, 0));

			const float scale = GetDpiScale(monitor());
			m_content.Arrange(winrt::Windows::UI::Xaml::RectHelper::FromCoordinatesAndDimensions(0, 0, x / scale, y / scale));
			return 0;
		}

		case WM_DPICHANGED:
			PositionWindow(*reinterpret_cast<RECT*>(lParam));
			break;

		// TODO: this causes the first element to be tab-navigated to when initially opening the window
		//case WM_SETFOCUS:
		//	SetFocus(m_interopWnd);
		//	return 0;

		case WM_NCDESTROY:
			HeapDeletePostNcDestroy();
			return 0;
		}

		return MessageWindow::MessageHandler(uMsg, wParam, lParam);
	}

	inline RECT CenterWindow(CenteringStrategy strategy, LONG width, LONG height, HMONITOR mon, POINT mouse)
	{
		MONITORINFO mi = { sizeof(mi) };
		if (!GetMonitorInfo(mon, &mi))
		{
			throw winrt::hresult_error(E_FAIL);
		}

		if (strategy == CenteringStrategy::Mouse)
		{
			return CenterRectOnMouse(width, height, mi.rcWork, mouse);
		}
		else
		{
			return CenterRectOnMonitor(width, height, mi.rcWork);
		}
	}

	RECT CalculateWindowPosition(CenteringStrategy strategy)
	{
		m_content.Measure(winrt::Windows::UI::Xaml::SizeHelper::FromDimensions(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));

		winrt::Windows::Foundation::Size size = m_content.DesiredSize();

		POINT point;
		winrt::check_bool(GetCursorPos(&point));
		const HMONITOR mon = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);

		const float scale = GetDpiScale(mon);
		size.Height *= scale;
		size.Width *= scale;

		return CenterWindow(strategy, static_cast<LONG>(std::round(size.Width)), static_cast<LONG>(std::round(size.Height)), mon, point);
	}

	void PositionWindow(const RECT &rect, bool showWindow = false)
	{
		winrt::check_bool(SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW : 0));
		winrt::check_bool(SetWindowPos(m_WindowHandle, Window::NullWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW | SWP_FRAMECHANGED : 0));
	}

public:
	// TODO: support multiple instances
	// todo: movable window
	template<typename... Args>
	inline XamlPageHost(HINSTANCE hInst, CenteringStrategy strategy, Args&&... args) :
		MessageWindow(GetClassName(), { }, hInst, WS_OVERLAPPED),
		// Don't construct the XAML stuff already.
		m_content(nullptr)
	{
		auto nativeSource = m_source.as<IDesktopWindowXamlSourceNative2>();
		winrt::check_hresult(nativeSource->AttachToWindow(m_WindowHandle));

		winrt::check_hresult(nativeSource->get_WindowHandle(m_interopWnd.put()));

		/*TODO: m_FilterCookie = RegisterThreadMessageFilter([nativeSource](const MSG &msg)
		{
			BOOL result;
			winrt::check_hresult(nativeSource->PreTranslateMessage(&msg, &result));
			return result;
		});*/

		m_content = T(std::forward<Args>(args)...);

		// Make sure T is a frameless page
		WINRT_ASSERT(m_content.try_as<winrt::TranslucentTB::Xaml::Pages::FramelessPage>());

		m_source.Content(m_content);

		SetTitle();
		m_TitleChangedToken.value = m_content.RegisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::TitleProperty(), { this, &XamlPageHost::SetTitle });

		// Magic that gives us shadows
		const MARGINS margins = { 1 };
		DwmExtendFrameIntoClientArea(m_WindowHandle, &margins);

		PositionWindow(CalculateWindowPosition(strategy), true);

		UpdateWindow(m_WindowHandle);
		SetFocus(m_WindowHandle);
	}

	inline T *operator ->()
	{
		return &m_content;
	}

	inline ~XamlPageHost()
	{
		m_content.UnregisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::TitleProperty(), std::exchange(m_TitleChangedToken.value, 0));
		m_content = nullptr;

		m_source.Close();
		m_source = nullptr;
	}

	XamlPageHost(const XamlPageHost &) = delete;
	XamlPageHost &operator =(const XamlPageHost &) = delete;
};
