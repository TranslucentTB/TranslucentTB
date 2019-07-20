#pragma once
#include "../windows/messagewindow.hpp"

#include <string>
#include <type_traits>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/TranslucentTB.Pages.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

namespace xaml = winrt::Windows::UI::Xaml;

enum class CenteringStrategy {
	Mouse,
	Monitor
};

template<typename T>
class XamlPageHost : public MessageWindow {
private:
	CenteringStrategy m_CenteringStrategy;
	xaml::Hosting::DesktopWindowXamlSource m_source;
	xaml::Media::ScaleTransform m_scaler;
	T m_content;
	int64_t m_TitleChangedToken;

	Window m_interopWnd;

	void RegisterCallbacks()
	{
		RegisterCallback(WM_SETFOCUS, [this](...)
		{
			// Forward keyboard focus to the XAML island.
			SetFocus(m_interopWnd);

			return 0;
		});

		RegisterCallback(WM_NCCALCSIZE, [](...)
		{
			return 0;
		});
	}

	RECT CenterRectOnMouse(LONG width, LONG height, const RECT &workArea, const POINT &mouse)
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

	RECT CenterRectOnMonitor(LONG width, LONG height, const RECT &workArea)
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

	RECT CenterWindow(LONG width, LONG height)
	{
		POINT point;
		winrt::check_bool(GetCursorPos(&point));
		HMONITOR mon = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);

		MONITORINFO mi = { sizeof(mi) };
		if (!GetMonitorInfo(mon, &mi))
		{
			throw winrt::hresult_error(E_FAIL);
		}

		if (m_CenteringStrategy == CenteringStrategy::Mouse)
		{
			return CenterRectOnMouse(width, height, mi.rcWork, point);
		}
		else
		{
			return CenterRectOnMonitor(width, height, mi.rcWork);
		}
	}

	// TODO: be able to use this method for dpi changes too
	void UpdateSize()
	{
		m_content.Measure(xaml::SizeHelper::FromDimensions(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));

		const winrt::Windows::Foundation::Size size = m_content.DesiredSize();

		auto rect = CenterWindow(size.Width, size.Height);

		winrt::check_bool(SetWindowPos(m_WindowHandle, Window::NullWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW | SWP_FRAMECHANGED));
		winrt::check_bool(SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, size.Width, size.Height, SWP_SHOWWINDOW));
	}

	void SetTitle(...)
	{
		SetWindowText(m_WindowHandle, m_content.Title().c_str());
	}

public:
	// TODO: support multiple instances
	// TODO: dpi awareness
	// TODO: better class name lol
	// todo: movable window
	template<typename... Args>
	inline XamlPageHost(HINSTANCE hInst, CenteringStrategy strategy, Args&&... args) :
		MessageWindow(L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", { }, hInst, WS_OVERLAPPED),
		m_CenteringStrategy(strategy),
		// Don't construct the XAML stuff already.
		m_scaler(nullptr),
		m_content(nullptr)
	{
		auto nativeSource = m_source.as<IDesktopWindowXamlSourceNative>();
		winrt::check_hresult(nativeSource->AttachToWindow(m_WindowHandle));

		winrt::check_hresult(nativeSource->get_WindowHandle(m_interopWnd.put()));

		m_scaler = { };
		m_content = T(std::forward<Args>(args)...);

		// Make sure T is a frameless page
		WINRT_ASSERT(m_content.try_as<winrt::TranslucentTB::Pages::FramelessPage>());

		m_content.RenderTransform(m_scaler);

		m_source.Content(m_content);

		SetTitle();
		m_TitleChangedToken = m_content.RegisterPropertyChangedCallback(winrt::TranslucentTB::Pages::FramelessPage::TitleProperty(), { this, &XamlPageHost::SetTitle });

		// Magic that gives us shadows
		const MARGINS margins = { 1 };
		DwmExtendFrameIntoClientArea(m_WindowHandle, &margins);

		RegisterCallbacks();

		UpdateSize();

		UpdateWindow(m_WindowHandle);
		SetFocus(m_WindowHandle);
	}

	inline T *operator ->()
	{
		return &m_content;
	}

	inline ~XamlPageHost()
	{
		m_content.UnregisterPropertyChangedCallback(winrt::TranslucentTB::Pages::FramelessPage::TitleProperty(), m_TitleChangedToken);
		m_scaler = nullptr;
		m_content = nullptr;

		m_source.Close();
		m_source = nullptr;
	}

	XamlPageHost(const XamlPageHost &) = delete;
	XamlPageHost &operator =(const XamlPageHost &) = delete;
};