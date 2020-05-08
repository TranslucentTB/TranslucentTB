#include "basexamlpagehost.hpp"
#include <ShellScalingApi.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

#include "../ProgramLog/error/win32.hpp"

float BaseXamlPageHost::GetDpiScale(HMONITOR mon)
{
	UINT dpiX, dpiY;
	if (const HRESULT hr = GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY); SUCCEEDED(hr))
	{
		return static_cast<float>(dpiX) / USER_DEFAULT_SCREEN_DPI;
	}
	else
	{
		HresultHandle(hr, spdlog::level::warn, L"Failed to get monitor DPI");
		return 1.0f;
	}
}

LRESULT BaseXamlPageHost::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)

{
	switch (uMsg)
	{
	case WM_NCCALCSIZE:
		return 0;

	case WM_SIZE:
	{
		const int x = LOWORD(lParam);
		const int y = HIWORD(lParam);
		if (!SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, x, y, 0))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to set position of interop window.");
		}

		const float scale = GetDpiScale(monitor());
		ArrangeContent(winrt::Windows::UI::Xaml::RectHelper::FromCoordinatesAndDimensions(0, 0, x / scale, y / scale));
		return 0;
	}

	case WM_DPICHANGED:
		PositionWindow(*reinterpret_cast<RECT *>(lParam));
		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
		{
			return 0;
		}
		else
		{
			break;
		}

	case WM_CLOSE:
		return 0;

	case WM_NCDESTROY:
		HeapDeletePostNcDestroy();
		return 0;
	}

	return MessageWindow::MessageHandler(uMsg, wParam, lParam);
}

RECT BaseXamlPageHost::CalculateWindowPosition(winrt::Windows::Foundation::Size size)
{
	// get primary monitor
	const HMONITOR mon = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi = { sizeof(mi) };
	if (!GetMonitorInfo(mon, &mi))
	{
		// no error info provided
		throw winrt::hresult_error(E_FAIL);
	}

	const float scale = GetDpiScale(mon);
	size.Height *= scale;
	size.Width *= scale;

	RECT temp;
	temp.left = (mi.rcWork.right - mi.rcWork.left - size.Width) / 2 + mi.rcWork.left;
	temp.right = temp.left + size.Width;

	temp.top = (mi.rcWork.bottom - mi.rcWork.top - size.Height) / 2 + mi.rcWork.top;
	temp.bottom = temp.top + size.Height;

	return temp;
}

void BaseXamlPageHost::PositionWindow(const RECT &rect, bool showWindow)
{
	winrt::check_bool(SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW : 0));
	winrt::check_bool(SetWindowPos(m_WindowHandle, Window::NullWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW | SWP_FRAMECHANGED : 0));
}

BaseXamlPageHost::BaseXamlPageHost(Util::null_terminated_wstring_view className, HINSTANCE hInst) :
	MessageWindow(className, { }, hInst, WS_OVERLAPPED)
{
	auto nativeSource = m_source.as<IDesktopWindowXamlSourceNative2>();
	winrt::check_hresult(nativeSource->AttachToWindow(m_WindowHandle));

	winrt::check_hresult(nativeSource->get_WindowHandle(m_interopWnd.put()));
}
