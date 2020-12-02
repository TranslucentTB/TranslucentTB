#pragma once
#include "../windows/messagewindow.hpp"

class XamlDragRegion final : public MessageWindow {
private:
	void HandleClick(UINT msg, LPARAM lParam);
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

public:
	XamlDragRegion(WindowClass &classRef, Window parent);
};
