#include "basexamlpagehost.hpp"
#include <ShellScalingApi.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

#include "win32.hpp"
#include "../ProgramLog/error/win32.hpp"

void BaseXamlPageHost::UpdateFrame()
{
	// Magic that gives us shadows
	// we use the top side because any other side would cause a single line of white pixels to
	// suddenly flash when resizing the color picker.
	// can't use 0, that does nothing
	// or -1: turns it full white.
	const MARGINS margins = { 0, 0, 1, 0 };
	HresultVerify(DwmExtendFrameIntoClientArea(m_WindowHandle, &margins), spdlog::level::warn, L"Failed to extend frame into client area");
}

HMONITOR BaseXamlPageHost::GetInitialMonitor(POINT &cursor, xaml_startup_position position)
{
	if (position == xaml_startup_position::mouse)
	{
		if (!GetCursorPos(&cursor))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to get cursor position");
		}
	}

	return MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
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

void BaseXamlPageHost::CalculateInitialPosition(int &x, int &y, int width, int height, POINT cursor, const RECT &workArea, xaml_startup_position position) noexcept
{
	if (position == xaml_startup_position::mouse)
	{
		// Center on the mouse
		x = cursor.x - (width / 2);
		y = cursor.y - (height / 2);

		AdjustWindowPosition(x, y, width, height, workArea);
	}
	else
	{
		x = ((workArea.right - workArea.left - width) / 2) + workArea.left;
		y = ((workArea.bottom - workArea.top - height) / 2) + workArea.top;
	}
}

bool BaseXamlPageHost::AdjustWindowPosition(int &x, int &y, int width, int height, const RECT &workArea) noexcept
{
	RECT coords = { x, y, x + width, y + height };
	if (win32::RectFitsInRect(workArea, coords))
	{
		// It fits, nothing to do.
		return false;
	}

	const bool rightDoesntFits = coords.right > workArea.right;
	const bool leftDoesntFits = coords.left < workArea.left;
	const bool bottomDoesntFits = coords.bottom > workArea.bottom;
	const bool topDoesntFits = coords.top < workArea.top;

	if ((rightDoesntFits && leftDoesntFits) || (bottomDoesntFits && topDoesntFits))
	{
		// Doesn't fits in the monitor work area :(
		return true;
	}

	// Offset the rect so that it is completely in the work area
	int x_offset = 0;
	if (rightDoesntFits)
	{
		x_offset = workArea.right - coords.right; // Negative offset
	}
	else if (leftDoesntFits)
	{
		x_offset = workArea.left - coords.left;
	}

	int y_offset = 0;
	if (bottomDoesntFits)
	{
		y_offset = workArea.bottom - coords.bottom; // Negative offset
	}
	else if (topDoesntFits)
	{
		y_offset = workArea.top - coords.top;
	}

	win32::OffsetRect(coords, x_offset, y_offset);
	x = coords.left;
	y = coords.top;

	return true;
}

LRESULT BaseXamlPageHost::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCALCSIZE:
		return 0;

	case WM_DWMCOMPOSITIONCHANGED:
		UpdateFrame();
		return 0;

	case WM_SETFOCUS:
		SetFocus(m_interopWnd);
		return 0;

	case WM_DPICHANGED:
	{
		const auto newRect = reinterpret_cast<RECT *>(lParam);

		ResizeWindow(newRect->left, newRect->top, newRect->right - newRect->left, newRect->bottom - newRect->top, true);
		return 0;
	}
	}

	return MessageWindow::MessageHandler(uMsg, wParam, lParam);
}

void BaseXamlPageHost::ResizeWindow(int x, int y, int width, int height, bool move, UINT flags)
{
	if (!SetWindowPos(m_interopWnd, nullptr, 0, 0, width, height, flags | SWP_NOACTIVATE | SWP_NOZORDER)) [[unlikely]]
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to set interop window position");
	}

	if (!SetWindowPos(m_WindowHandle, nullptr, x, y, width, height, flags | (move ? 0 : SWP_NOMOVE) | SWP_NOACTIVATE)) [[unlikely]]
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to set host window position");
	}
}

void BaseXamlPageHost::PositionDragRegion(wf::Rect position, wf::Rect buttonsRegion, UINT flags)
{
	if (const auto wndRect = rect())
	{
		m_DragRegion.Position(*wndRect, position, buttonsRegion, flags);
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

bool BaseXamlPageHost::PaintBackground(HDC dc, const RECT &target, winrt::Windows::UI::Color col)
{
	if (!m_BackgroundBrush || m_BackgroundColor != col)
	{
		m_BackgroundBrush.reset(CreateSolidBrush(RGB(col.R, col.G, col.B)));
	}

	if (m_BackgroundBrush)
	{
		m_BackgroundColor = col;

		// buffered paints overwrite the titlebar for some reason
		HDC opaqueDc;
		BP_PAINTPARAMS params = { sizeof(params), BPPF_NOCLIP | BPPF_ERASE };
		HPAINTBUFFER buf = BeginBufferedPaint(dc, &target, BPBF_TOPDOWNDIB, &params, &opaqueDc);
		if (buf && opaqueDc)
		{
			if (!FillRect(opaqueDc, &target, m_BackgroundBrush.get()))
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to fill rectangle.");
			}

			HresultVerify(BufferedPaintSetAlpha(buf, nullptr, 255), spdlog::level::warn, L"Failed to set buffered paint alpha");

			const HRESULT hr = EndBufferedPaint(buf, true);
			if (SUCCEEDED(hr))
			{
				return true;
			}
			else
			{
				HresultHandle(hr, spdlog::level::warn, L"Failed to end buffered paint");
			}
		}
		else
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to begin buffered paint");
		}
	}

	return false;
}

BaseXamlPageHost::BaseXamlPageHost(WindowClass &classRef, WindowClass &dragRegionClass) :
	MessageWindow(classRef, { }),
	m_DragRegion(dragRegionClass, m_WindowHandle)
{
	UpdateFrame();

	auto nativeSource = m_source.as<IDesktopWindowXamlSourceNative2>();
	HresultVerify(nativeSource->AttachToWindow(m_WindowHandle), spdlog::level::critical, L"Failed to attach DesktopWindowXamlSource");
	HresultVerify(nativeSource->get_WindowHandle(m_interopWnd.put()), spdlog::level::critical, L"Failed to get interop window handle");

	m_focusRevoker = m_source.TakeFocusRequested(winrt::auto_revoke, [](const wuxh::DesktopWindowXamlSource &sender, const wuxh::DesktopWindowXamlSourceTakeFocusRequestedEventArgs &args)
	{
		const auto request = args.Request();
		const auto reason = request.Reason();
		if (reason == wuxh::XamlSourceFocusNavigationReason::First || reason == wuxh::XamlSourceFocusNavigationReason::Last)
		{
			// just cycle back to beginning
			sender.NavigateFocus(request);
		}
	});
}
