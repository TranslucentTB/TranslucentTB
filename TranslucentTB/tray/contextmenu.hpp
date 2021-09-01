#pragma once
#include "basecontextmenu.hpp"
#include "arch.h"
#include <shellscalingapi.h>
#include <utility>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Controls.h>
#include "redefgetcurrenttime.h"

#include "win32.hpp"
#include "../ProgramLog/error/win32.hpp"

template<typename T>
class ContextMenu : public BaseContextMenu {
private:
	T m_Page;

	bool m_UseXamlContextMenu;

	void ShowTooltip(bool visible)
	{
		if (const auto tooltip = wuxc::ToolTipService::GetToolTip(m_Page).try_as<wuxc::ToolTip>())
		{
			tooltip.IsOpen(visible);
		}
	}

protected:
	constexpr const T &page() const noexcept
	{
		return m_Page;
	}

	template<typename... Args>
	ContextMenu(Args&&... args) : m_Page(std::forward<Args>(args)...), m_UseXamlContextMenu(win32::IsAtLeastBuild(22000))
	{
		source().Content(m_Page);
	}

	void ShowAt(RECT rect, POINT pt)
	{
		if (const auto flyout = m_Page.ContextFlyout().try_as<wuxc::MenuFlyout>())
		{
			if (m_UseXamlContextMenu)
			{
				MoveHiddenWindow(rect);
				const float scaleFactor = static_cast<float>(GetDpiForWindow(m_WindowHandle)) / USER_DEFAULT_SCREEN_DPI;

				flyout.ShowAt(m_Page, {
					(pt.x - rect.left) / scaleFactor,
					(pt.y - rect.top) / scaleFactor
				});
			}
			else
			{
				ShowClassicContextMenu(flyout, pt);
			}
		}
	}

	void ShowTooltip(RECT rect)
	{
		MoveHiddenWindow(rect);
		ShowTooltip(true);
	}

	void HideTooltip()
	{
		ShowTooltip(false);
	}

	~ContextMenu()
	{
		source().Content(nullptr);
		m_Page = nullptr;
	}
};
