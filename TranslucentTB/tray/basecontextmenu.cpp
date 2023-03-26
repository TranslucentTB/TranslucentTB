#include "basecontextmenu.hpp"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Automation.Peers.h>
#include <winrt/Windows.UI.Xaml.Automation.Provider.h>

#include "win32.hpp"
#include "../ProgramLog/error/win32.hpp"

UINT BaseContextMenu::GetNextClickableId() noexcept
{
	// skip over 0 because TrackPopupMenu returning 0 means the menu was dismissed
	// or an error occured. We don't want to trigger an entry when the menu is dismissed,
	// and we can't distinguish menu dismisses from proper clicks.
	if (m_ContextMenuClickableId == 0)
	{
		m_ContextMenuClickableId = 1;
	}

	return m_ContextMenuClickableId++;
}

wil::unique_hmenu BaseContextMenu::BuildContextMenuInner(const wfc::IVector<wuxc::MenuFlyoutItemBase> &items)
{
	wil::unique_hmenu menu(CreatePopupMenu());
	if (menu)
	{
		UINT position = 0;
		for (const wuxc::MenuFlyoutItemBase item : items)
		{
			if (item.Visibility() != wux::Visibility::Visible)
			{
				continue;
			}

			wil::unique_hmenu subMenu;
			winrt::hstring menuText;
			MENUITEMINFO itemInfo = {
				.cbSize = sizeof(itemInfo)
			};

			if (const auto menuItem = item.try_as<wuxc::MenuFlyoutItem>())
			{
				// keep a reference alive for dwItemData
				m_Items.insert(menuItem);
				menuText = menuItem.Text();

				itemInfo.fMask = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_STRING;
				itemInfo.fState = menuItem.IsEnabled() ? MFS_ENABLED : MFS_DISABLED;
				itemInfo.wID = GetNextClickableId();
				itemInfo.dwTypeData = const_cast<wchar_t *>(menuText.c_str());
				itemInfo.dwItemData = reinterpret_cast<ULONG_PTR>(winrt::get_abi(menuItem));

				if (const auto radioItem = menuItem.try_as<muxc::RadioMenuFlyoutItem>())
				{
					itemInfo.fState |= radioItem.IsChecked() ? MFS_CHECKED : MFS_UNCHECKED;
					itemInfo.fMask |= MIIM_FTYPE;
					itemInfo.fType = MFT_RADIOCHECK;
				}
				else if (const auto toggleItem = menuItem.try_as<wuxc::ToggleMenuFlyoutItem>())
				{
					itemInfo.fState |= toggleItem.IsChecked() ? MFS_CHECKED : MFS_UNCHECKED;
				}
			}
			else if (const auto subItem = item.try_as<wuxc::MenuFlyoutSubItem>())
			{
				subMenu = BuildContextMenuInner(subItem.Items());
				menuText = subItem.Text();

				itemInfo.fMask = MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;
				itemInfo.fState = subItem.IsEnabled() ? MFS_ENABLED : MFS_DISABLED;
				itemInfo.hSubMenu = subMenu.get();
				itemInfo.dwTypeData = const_cast<wchar_t *>(menuText.c_str());
			}
			else if (const auto separator = item.try_as<wuxc::MenuFlyoutSeparator>())
			{
				itemInfo.fMask = MIIM_FTYPE;
				itemInfo.fType = MFT_SEPARATOR;
			}
			else
			{
				MessagePrint(spdlog::level::warn, L"Unknown flyout item type");
				continue;
			}

			if (InsertMenuItem(menu.get(), position, true, &itemInfo))
			{
				subMenu.release(); // ownership transferred to system
				++position;
			}
			else
			{
				LastErrorHandle(spdlog::level::warn, L"Failed to insert menu item");
			}
		}
	}
	else
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to create popup menu");
	}

	return menu;
}

HMENU BaseContextMenu::BuildClassicContextMenu(const wuxc::MenuFlyout &flyout)
{
	m_ContextMenu = BuildContextMenuInner(flyout.Items());
	return m_ContextMenu.get();
}

void BaseContextMenu::TriggerClassicContextMenuItem(UINT item)
{
	MENUITEMINFO itemInfo = {
		.cbSize = sizeof(itemInfo),
		.fMask = MIIM_DATA
	};

	if (GetMenuItemInfo(m_ContextMenu.get(), item, false, &itemInfo))
	{
		wuxc::MenuFlyoutItem menuItem(nullptr);
		winrt::copy_from_abi(menuItem, reinterpret_cast<void *>(itemInfo.dwItemData));

		if (menuItem)
		{
			// muxc::RadioMenuFlyoutItem secretely inherits from wuxc::ToggleMenuFlyoutItem,
			// so try_as succeeds and the automation peer works still.
			if (const auto toggleItem = menuItem.try_as<wuxc::ToggleMenuFlyoutItem>())
			{
				wux::Automation::Peers::ToggleMenuFlyoutItemAutomationPeer(toggleItem).Toggle();
			}
			else
			{
				wux::Automation::Peers::MenuFlyoutItemAutomationPeer(menuItem).Invoke();
			}
		}
	}
	else
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to get menu item info");
	}
}

void BaseContextMenu::CleanupClassicContextMenu()
{
	m_ContextMenu.reset();
	m_Items.clear();
}

bool BaseContextMenu::ShouldUseXamlMenu()
{
	if (!m_UseXamlMenu)
	{
		static const bool xamlMenuWorks = []
		{
			if (win32::IsAtLeastBuild(19045))
			{
				// Windows 10 22H2 and up (including Windows 11) - always works
				return true;
			}
			else if (win32::IsAtLeastBuild(19041))
			{
				// Windows 10 21H2, 21H1, 20H2, 2004 - requires KB5007253 (which is revision number 1387 on all of those)
				if (const auto [version, hr] = win32::GetWindowsBuild(); SUCCEEDED(hr))
				{
					return version.Revision >= 1387;
				}
				else
				{
					return false;
				}
			}
			else
			{
				// older than 2004 - always broken
				return false;
			}
		}();

		return xamlMenuWorks;
	}
	else
	{
		return *m_UseXamlMenu;
	}
}

void BaseContextMenu::ShowClassicContextMenu(const wuxc::MenuFlyout &flyout, POINT pt)
{
	const auto guard = wil::scope_exit([this]
	{
		CleanupClassicContextMenu();
	});

	if (const auto menu = BuildClassicContextMenu(flyout))
	{
		SetLastError(NO_ERROR);
		const unsigned int item = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, m_WindowHandle, nullptr);
		if (item)
		{
			TriggerClassicContextMenuItem(item);
		}
		else if (const DWORD lastErr = GetLastError(); lastErr != NO_ERROR) // can return 0 if the menu is dismissed
		{
			HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::warn, L"Failed to open context menu.");
		}
	}
}

void BaseContextMenu::MoveHiddenWindow(RECT &rect)
{
	HMONITOR mon = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
	MONITORINFO info = { sizeof(info) };
	if (mon && GetMonitorInfo(mon, &info))
	{
		// if it's in the taskbar (completely outside work area), oversize the window
		// to allow the MenuFlyout to put itself in the work area
		if (!win32::RectFitsInRect(info.rcWork, rect))
		{
			ExtendIntoArea(rect, info.rcWork);
		}

		const int width = rect.right - rect.left;
		const int height = rect.bottom - rect.top;
		if (!SetWindowPos(m_WindowHandle, nullptr, rect.left, rect.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to move window");
		}

		if (!SetWindowPos(m_InteropWnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to resize interop window");
		}
	}
}

BaseContextMenu::BaseContextMenu()
{
	const auto nativeSource = m_Source.as<IDesktopWindowXamlSourceNative2>();
	HresultVerify(nativeSource->AttachToWindow(m_WindowHandle), spdlog::level::critical, L"Failed to attach DesktopWindowXamlSource");
	HresultVerify(nativeSource->get_WindowHandle(m_InteropWnd.put()), spdlog::level::critical, L"Failed to get interop window handle");

	// This *needs* to use SetWindowPos, don't ask me why
	if (!SetWindowPos(m_InteropWnd, nullptr, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE))
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to show interop window");
	}
}

BaseContextMenu::~BaseContextMenu()
{
	m_Source.Close();
	m_Source = nullptr;
}

void BaseContextMenu::SetXamlContextMenuOverride(const std::optional<bool> &menuOverride)
{
	m_UseXamlMenu = menuOverride;
}

bool BaseContextMenu::PreTranslateMessage(const MSG &msg)
{
	if (const auto nativeSource = m_Source.try_as<IDesktopWindowXamlSourceNative2>())
	{
		BOOL result;
		const HRESULT hr = nativeSource->PreTranslateMessage(&msg, &result);
		if (SUCCEEDED(hr))
		{
			return result;
		}
		else
		{
			HresultHandle(hr, spdlog::level::warn, L"Failed to pre-translate message");
		}
	}

	return false;
}
