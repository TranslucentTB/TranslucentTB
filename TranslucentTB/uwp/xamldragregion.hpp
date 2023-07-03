#pragma once
#include "winrt.hpp"
#include <winrt/Windows.Foundation.h>

#include "../windows/messagewindow.hpp"

class XamlDragRegion final : public MessageWindow {
private:
	wf::Rect m_ButtonsRegion = { };
	bool m_Tracking = false;

	void HandleClick(UINT msg, LPARAM lParam) noexcept;
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void SetRegion(int width, int height, wf::Rect buttonsRegion);

public:
	XamlDragRegion(WindowClass &classRef, Window parent);
	void Position(const RECT &parentRect, wf::Rect position, wf::Rect buttonsRegion, UINT flags);
};
