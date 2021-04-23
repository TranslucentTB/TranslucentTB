#pragma once
#include "basecontextmenu.hpp"
#include "arch.h"
#include <shellscalingapi.h>
#include <utility>
#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/TranslucentTB.Xaml.Pages.h>
#include "redefgetcurrenttime.h"

#include "../ProgramLog/error/win32.hpp"

template<typename T>
requires std::derived_from<T, winrt::impl::base_one<T, winrt::TranslucentTB::Xaml::Pages::FlyoutPage>> // https://github.com/microsoft/cppwinrt/issues/609
class ContextMenu : public BaseContextMenu {
private:
	T m_Page;

	bool m_UseXamlContextMenu = false;

protected:
	constexpr const T &page() const noexcept
	{
		return m_Page;
	}

	template<typename... Args>
	ContextMenu(Args&&... args) : m_Page(std::forward<Args>(args)...)
	{
		source().Content(m_Page);
	}

	void ShowAt(RECT rect, POINT pt)
	{
		if (const auto monitor = MoveHiddenWindow(rect))
		{
			if (const auto flyout = m_Page.ContextFlyout().try_as<wuxc::MenuFlyout>())
			{
				if (m_UseXamlContextMenu)
				{
					UINT dpiX, dpiY;
					if (const HRESULT hr = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY); SUCCEEDED(hr))
					{
						const float scaleFactor = static_cast<float>(dpiX) / USER_DEFAULT_SCREEN_DPI;

						flyout.ShowAt(m_Page, {
							(pt.x - rect.left) / scaleFactor,
							(pt.y - rect.top) / scaleFactor
						});
					}
					else
					{
						HresultHandle(hr, spdlog::level::warn, L"Failed to get monitor DPI");
					}
				}
				else
				{
					ShowClassicContextMenu(flyout);
				}
			}
		}
	}

	void ShowTooltip(RECT rect)
	{
		MoveHiddenWindow(rect);
		m_Page.SetTooltipVisible(true);
	}

	void HideTooltip()
	{
		m_Page.SetTooltipVisible(false);
	}

	constexpr void ShouldUseXamlContextMenu(bool value) noexcept
	{
		m_UseXamlContextMenu = value;
	}

	~ContextMenu()
	{
		source().Content(nullptr);
		m_Page = nullptr;
	}
};
