#include "basexamlpagehost.hpp"
#include <ShellScalingApi.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

#include "../ProgramLog/error/win32.hpp"

using namespace winrt::Windows::UI::Xaml::Hosting;

bool BaseXamlPageHost::PreTranslateMessage(const MSG &msg)
{
	if (const auto source = m_source.try_as<IDesktopWindowXamlSourceNative2>())
	{
		BOOL result { };
		const HRESULT err = source->PreTranslateMessage(&msg, &result);
		if (SUCCEEDED(err))
		{
			return result;
		}
		else
		{
			HresultHandle(err, spdlog::level::warn, L"Failed to pre-translate message.");
		}
	}

	return false;
}

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

	case WM_DPICHANGED:
		PositionWindow(*reinterpret_cast<RECT *>(lParam));
		break;
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
	temp.left = static_cast<LONG>((mi.rcWork.right - mi.rcWork.left - size.Width) / 2) + mi.rcWork.left;
	temp.right = temp.left + static_cast<LONG>(size.Width);

	temp.top = static_cast<LONG>((mi.rcWork.bottom - mi.rcWork.top - size.Height) / 2) + mi.rcWork.top;
	temp.bottom = temp.top + static_cast<LONG>(size.Height);

	return temp;
}

void BaseXamlPageHost::PositionWindow(const RECT &rect, bool showWindow)
{
	winrt::check_bool(SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW : 0));
	winrt::check_bool(SetWindowPos(m_WindowHandle, Window::NullWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, showWindow ? SWP_SHOWWINDOW | SWP_FRAMECHANGED : 0));
}

void BaseXamlPageHost::PositionInteropWindow(int x, int y)
{
	if (!SetWindowPos(m_interopWnd, Window::NullWindow, 0, 0, x, y, 0))
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to set position of interop window.");
	}
}

void BaseXamlPageHost::Flash() noexcept
{
	FLASHWINFO fwi = {
		.cbSize = sizeof(fwi),
		.hwnd = m_WindowHandle,
		.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG
	};

	FlashWindowEx(&fwi);
}

BaseXamlPageHost::BaseXamlPageHost(Util::null_terminated_wstring_view className, HINSTANCE hInst) :
	MessageWindow(className, { }, hInst, WS_OVERLAPPED),
	m_manager(nullptr),
	m_source(nullptr)
{
	winrt::init_apartment(winrt::apartment_type::single_threaded);
	m_manager = WindowsXamlManager::InitializeForCurrentThread();
	m_source = { };

	auto nativeSource = m_source.as<IDesktopWindowXamlSourceNative2>();
	winrt::check_hresult(nativeSource->AttachToWindow(m_WindowHandle));

	winrt::check_hresult(nativeSource->get_WindowHandle(m_interopWnd.put()));

	m_focusRevoker = m_source.TakeFocusRequested(winrt::auto_revoke, [](const DesktopWindowXamlSource &sender, const DesktopWindowXamlSourceTakeFocusRequestedEventArgs &args)
	{
		// just cycle back to beginning
		sender.NavigateFocus(args.Request());
	});
}
