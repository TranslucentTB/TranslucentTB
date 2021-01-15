#include "xamldragregion.hpp"
#include <windowsx.h>
#include <wil/resource.h>

#include "../ProgramLog/error/win32.hpp"

void XamlDragRegion::HandleClick(UINT msg, LPARAM lParam) noexcept
{
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	if (ClientToScreen(m_WindowHandle, &pt))
	{
		ancestor(GA_PARENT).send_message(msg, HTCAPTION, MAKELPARAM(pt.x, pt.y));
	}
}

LRESULT XamlDragRegion::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		HandleClick(WM_NCLBUTTONDOWN, lParam);
		return 0;

	case WM_LBUTTONDBLCLK:
		HandleClick(WM_NCLBUTTONDBLCLK, lParam);
		return 0;

	case WM_LBUTTONUP:
		HandleClick(WM_NCLBUTTONUP, lParam);
		return 0;

	case WM_RBUTTONDOWN:
		HandleClick(WM_NCRBUTTONDOWN, lParam);
		return 0;

	case WM_RBUTTONDBLCLK:
		HandleClick(WM_NCRBUTTONDBLCLK, lParam);
		return 0;

	case WM_RBUTTONUP:
		HandleClick(WM_NCRBUTTONUP, lParam);
		return 0;

	default:
		return MessageWindow::MessageHandler(uMsg, wParam, lParam);
	}
}

void XamlDragRegion::SetRegion(int width, int height, wf::Rect buttonsRegion)
{
	if (buttonsRegion != m_ButtonsRegion)
	{
		if (buttonsRegion == wf::Rect { 0, 0, 0, 0 })
		{
			if (!SetWindowRgn(m_WindowHandle, nullptr, false)) [[unlikely]]
			{
				MessagePrint(spdlog::level::warn, L"Failed to clear window region");
				return;
			}
		}
		else
		{
			wil::unique_hrgn windowRgn(CreateRectRgn(0, 0, width, height));
			if (!windowRgn) [[unlikely]]
			{
				MessagePrint(spdlog::level::warn, L"Failed to create window region");
				return;
			}

			wil::unique_hrgn buttonsRgn(CreateRectRgn(
				static_cast<int>(buttonsRegion.X),
				static_cast<int>(buttonsRegion.Y),
				static_cast<int>(buttonsRegion.X + buttonsRegion.Width),
				static_cast<int>(buttonsRegion.Y + buttonsRegion.Height)
			));
			if (!buttonsRgn) [[unlikely]]
			{
				MessagePrint(spdlog::level::warn, L"Failed to create buttons region");
				return;
			}

			CombineRgn(windowRgn.get(), windowRgn.get(), buttonsRgn.get(), RGN_DIFF);

			if (SetWindowRgn(m_WindowHandle, windowRgn.get(), false))
			{
				windowRgn.release(); // the system owns the region now
			}
			else
			{
				MessagePrint(spdlog::level::warn, L"Failed to set window region");
				return;
			}
		}

		m_ButtonsRegion = buttonsRegion;
	}
}

XamlDragRegion::XamlDragRegion(WindowClass &classRef, Window parent) :
	MessageWindow(classRef, { }, WS_CHILD, WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, parent)
{
}

void XamlDragRegion::Position(const RECT &parentRect, wf::Rect position, wf::Rect buttonsRegion, UINT flags)
{
	const auto newX = static_cast<int>(position.X);
	const auto newY = static_cast<int>(position.Y);
	const auto newHeight = static_cast<int>(position.Height);
	const auto newWidth = static_cast<int>(position.Width);

	if (const auto dragRegionRect = rect())
	{
		const auto x = dragRegionRect->left - parentRect.left;
		const auto y = dragRegionRect->top - parentRect.top;
		const auto width = dragRegionRect->right - dragRegionRect->left;
		const auto height = dragRegionRect->bottom - dragRegionRect->top;
		if (x != newX || y != newY || height != newHeight || width != newWidth) [[unlikely]]
		{
			if (!SetWindowPos(m_WindowHandle, HWND_TOP, newX, newY, newWidth, newHeight, flags | SWP_NOACTIVATE)) [[unlikely]]
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to set drag region window position");
			}

			if (!SetLayeredWindowAttributes(m_WindowHandle, 0, 255, LWA_ALPHA)) [[unlikely]]
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to set drag region window attributes");
			}
		}
	}

	SetRegion(newWidth, newHeight, buttonsRegion);
}
