#pragma once
#include "basecontextmenu.hpp"
#include "arch.h"
#include <shellscalingapi.h>
#include <utility>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include "redefgetcurrenttime.h"

template<typename T>
class ContextMenu : public BaseContextMenu {
private:
	T m_Page;
	winrt::event_token m_FlyoutOpenedToken, m_FlyoutClosedToken;

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
	ContextMenu(Args&&... args) : m_Page(std::forward<Args>(args)...)
	{
		source().Content(m_Page);

		// prevent the shy taskbar from auto dismissing while the user is in the menu
		const auto flyout = m_Page.ContextFlyout();
		m_FlyoutOpenedToken = flyout.Opened([wnd = m_WindowHandle](const wf::IInspectable&, const wf::IInspectable&)
		{
			NotifyWinEvent(EVENT_SYSTEM_MENUPOPUPSTART, wnd, OBJID_WINDOW, CHILDID_SELF);
		});
		m_FlyoutClosedToken = flyout.Closed([wnd = m_WindowHandle](const wf::IInspectable&, const wf::IInspectable&)
		{
			NotifyWinEvent(EVENT_SYSTEM_MENUPOPUPEND, wnd, OBJID_WINDOW, CHILDID_SELF);
		});
	}

	void ShowAt(RECT rect, POINT pt)
	{
		if (const auto flyout = m_Page.ContextFlyout().try_as<wuxc::MenuFlyout>())
		{
			if (ShouldUseXamlMenu())
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

		const auto flyout = m_Page.ContextFlyout();
		flyout.Closed(m_FlyoutClosedToken);
		flyout.Opened(m_FlyoutOpenedToken);
		m_Page = nullptr;
	}
};
