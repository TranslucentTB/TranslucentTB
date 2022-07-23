#pragma once
#include "arch.h"
#include <unordered_set>
#include <windef.h>
#include <wil/resource.h>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

#include "../windows/messagewindow.hpp"

class BaseContextMenu : public virtual MessageWindow {
private:
	wuxh::DesktopWindowXamlSource m_Source;
	Window m_InteropWnd;

	wil::unique_hmenu m_ContextMenu;
	std::unordered_set<wuxc::MenuFlyoutItem> m_Items;
	UINT m_ContextMenuClickableId = 0;

	static constexpr void ExtendIntoArea(RECT &toExtend, const RECT &area) noexcept
	{
		if (toExtend.left >= area.right)
		{
			toExtend.left = area.right - 1;
		}

		if (toExtend.right <= area.left)
		{
			toExtend.right = area.left + 1;
		}

		if (toExtend.top >= area.bottom)
		{
			toExtend.top = area.bottom - 1;
		}

		if (toExtend.bottom <= area.top)
		{
			toExtend.bottom = area.top + 1;
		}
	}

	UINT GetNextClickableId() noexcept;
	wil::unique_hmenu BuildContextMenuInner(const wfc::IVector<wuxc::MenuFlyoutItemBase> &items);
	HMENU BuildClassicContextMenu(const wuxc::MenuFlyout &flyout);
	void TriggerClassicContextMenuItem(UINT item);
	void CleanupClassicContextMenu();

protected:
	static bool ShouldUseXamlMenu();
	void ShowClassicContextMenu(const wuxc::MenuFlyout &flyout, POINT pt);

	// pass by ref to update the caller's value to represent the new window rect
	// if it needed adjustment
	void MoveHiddenWindow(RECT &rect);

	virtual void RefreshMenu() = 0;

	BaseContextMenu();
	~BaseContextMenu();

	constexpr const wuxh::DesktopWindowXamlSource &source() noexcept
	{
		return m_Source;
	}

public:
	bool PreTranslateMessage(const MSG &msg);
};
