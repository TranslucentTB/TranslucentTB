#include "xamldragregion.hpp"
#include <windowsx.h>

void XamlDragRegion::HandleClick(UINT msg, LPARAM lParam)
{
	const POINT clientPt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	POINT screenPt = clientPt;
	if (ClientToScreen(m_WindowHandle, &screenPt))
	{
		ancestor(GA_PARENT).send_message(msg, HTCAPTION, MAKELPARAM(screenPt.x, screenPt.y));
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

XamlDragRegion::XamlDragRegion(WindowClass &classRef, Window parent) :
	MessageWindow(classRef, { }, WS_CHILD, WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP, parent)
{
}
