#include "pch.h"
#include "../Controls/RadioMenuFlyoutItem.h"

#include "TrayFlyoutPage.h"
#if __has_include("Pages/TrayFlyoutPage.g.cpp")
#include "Pages/TrayFlyoutPage.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	TrayFlyoutPage::TrayFlyoutPage(bool hasPackageIdentity)
	{
		InitializeComponent();

		if (!hasPackageIdentity)
		{
			StartupState().Visibility(wux::Visibility::Collapsed);
		}
	}

	void TrayFlyoutPage::SetTaskbarSettings(const txmp::TaskbarState &state, const txmp::TaskbarAppearance &appearance)
	{
		if (const auto submenu = GetSubMenuForState(state))
		{
			bool enabled = true;
			if (const auto optAppearance = appearance.try_as<txmp::OptionalTaskbarAppearance>())
			{
				enabled = optAppearance.Enabled();
			}

			for (const auto item : submenu.Items())
			{
				const auto tag = item.Tag();
				if (const auto radioItem = item.try_as<Controls::implementation::RadioMenuFlyoutItem>())
				{
					if (tag.try_as<txmp::AccentState>() == appearance.Accent())
					{
						radioItem->IsChecked(true);
					}
					else
					{
						// workaround for bug in classical context menu where the previous item isn't
						// unchecked correctly because VisualTreeHelper::GetParent returns null.
						// so manually uncheck it instead.
						radioItem->ProgrammaticUncheck();
					}

					radioItem->IsEnabled(enabled);
				}
				else if (const auto toggleItem = item.try_as<wuxc::ToggleMenuFlyoutItem>())
				{
					const auto stringTag = tag.try_as<hstring>();
					if (stringTag == L"Enabled")
					{
						toggleItem.IsChecked(enabled);
					}
					else if (stringTag == L"ShowPeek")
					{
						toggleItem.IsChecked(appearance.ShowPeek());
						toggleItem.IsEnabled(enabled);
					}
				}
				else if (tag.try_as<hstring>() == L"Color")
				{
					item.IsEnabled(enabled && appearance.Accent() != txmp::AccentState::Normal);
				}
			}
		}
	}

	void TrayFlyoutPage::SetLogLevel(const txmp::LogLevel &level)
	{
		for (const auto item : LogLevelSubMenu().Items())
		{
			if (const auto radioItem = item.try_as<Controls::implementation::RadioMenuFlyoutItem>())
			{
				if (radioItem->Tag().try_as<txmp::LogLevel>() == level)
				{
					radioItem->IsChecked(true);
				}
				else
				{
					// workaround for bug in classical context menu where the previous item isn't
					// unchecked correctly because VisualTreeHelper::GetParent returns null.
					// so manually uncheck it instead.
					radioItem->ProgrammaticUncheck();
				}
			}
		}
	}

	void TrayFlyoutPage::SetDisableSavingSettings(const bool &disabled)
	{
		DisableSavingSettings().IsChecked(disabled);
	}

	void TrayFlyoutPage::SetStartupState(const wf::IReference<Windows::ApplicationModel::StartupTaskState> &state)
	{
		const auto startup = StartupState();
		if (state)
		{
			const auto stateUnbox = state.Value();

			using enum Windows::ApplicationModel::StartupTaskState;
			startup.IsChecked(stateUnbox == Enabled || stateUnbox == EnabledByPolicy);
			startup.IsEnabled(stateUnbox == Disabled || stateUnbox == DisabledByUser || stateUnbox == Enabled);
		}
		else
		{
			startup.IsChecked(false);
			startup.IsEnabled(false);
		}
	}

	void TrayFlyoutPage::AppearanceClicked(const IInspectable &sender, const wux::RoutedEventArgs &)
	{
		if (const auto item = sender.try_as<wuxc::MenuFlyoutItemBase>())
		{
			if (const auto submenu = GetItemParent(item))
			{
				if (const auto tag = submenu.Tag().try_as<txmp::TaskbarState>())
				{
					const auto appearance = BuildAppearanceFromSubMenu(submenu);

					// workaround for bug in classical context menu where the previous item isn't
					// unchecked correctly because VisualTreeHelper::GetParent returns null.
					// so if the user clicked on a radio flyout item, then force
					// the accent to the value that the clicked item is tagged with.
					if (const auto radioItem = item.try_as<Controls::RadioMenuFlyoutItem>())
					{
						if (const auto accent = radioItem.Tag().try_as<txmp::AccentState>())
						{
							appearance.Accent(*accent);
						}
					}

					m_TaskbarSettingsChangedDelegate(*tag, appearance);
				}
			}
		}
	}

	void TrayFlyoutPage::ColorClicked(const IInspectable &sender, const wux::RoutedEventArgs &)
	{
		if (const auto item = sender.try_as<wuxc::MenuFlyoutItemBase>())
		{
			if (const auto submenu = GetItemParent(item))
			{
				if (const auto tag = submenu.Tag().try_as<txmp::TaskbarState>())
				{
					m_ColorRequestedDelegate(*tag);
				}
			}
		}
	}

	void TrayFlyoutPage::OpenLogFileClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_OpenLogFileRequestedDelegate();
	}

	void TrayFlyoutPage::LogLevelClicked(const IInspectable &sender, const wux::RoutedEventArgs &)
	{
		if (const auto item = sender.try_as<wuxc::MenuFlyoutItemBase>())
		{
			if (const auto tag = item.Tag().try_as<txmp::LogLevel>())
			{
				m_LogLevelChangedDelegate(*tag);
			}
		}
	}

	void TrayFlyoutPage::DumpDynamicStateClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_DumpDynamicStateRequestedDelegate();
	}

	void TrayFlyoutPage::EditSettingsClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_EditSettingsRequestedDelegate();
	}

	void TrayFlyoutPage::ResetSettingsClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_ResetSettingsRequestedDelegate();
	}

	void TrayFlyoutPage::DisableSavingSettingsClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_DisableSavingSettingsChangedDelegate(DisableSavingSettings().IsChecked());
	}

	void TrayFlyoutPage::HideTrayClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_HideTrayRequestedDelegate();
	}

	void TrayFlyoutPage::ResetDynamicStateClicked(const IInspectable &, const wux::RoutedEventArgs&)
	{
		m_ResetDynamicStateRequestedDelegate();
	}

	void TrayFlyoutPage::CompactThunkHeapClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_CompactThunkHeapRequestedDelegate();
	}

	void TrayFlyoutPage::StartupClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_StartupStateChangedDelegate();
	}

	void TrayFlyoutPage::TipsAndTricksClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_TipsAndTricksRequestedDelegate();
	}

	void TrayFlyoutPage::AboutClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_AboutRequestedDelegate();
	}

	void TrayFlyoutPage::ExitClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_ExitRequestedDelegate();
	}

	wuxc::MenuFlyoutSubItem TrayFlyoutPage::GetContainingSubMenu(const wuxc::MenuFlyoutItemBase &item, const wuxc::MenuFlyoutSubItem &subItem)
	{
		for (const auto menuItem : subItem.Items())
		{
			if (menuItem == item)
			{
				return subItem;
			}
			else if (const auto subSubMenu = menuItem.try_as<wuxc::MenuFlyoutSubItem>())
			{
				if (const auto parent = GetContainingSubMenu(item, subSubMenu))
				{
					return parent;
				}
			}
		}

		return nullptr;
	}

	wuxc::MenuFlyoutSubItem TrayFlyoutPage::GetItemParent(const wuxc::MenuFlyoutItemBase &item)
	{
		for (const auto menuItem : ContextMenu().Items())
		{
			if (const auto subMenu = menuItem.try_as<wuxc::MenuFlyoutSubItem>())
			{
				if (const auto parent = GetContainingSubMenu(item, subMenu))
				{
					return parent;
				}
			}
		}

		return nullptr;
	}

	txmp::TaskbarAppearance TrayFlyoutPage::BuildAppearanceFromSubMenu(const wuxc::MenuFlyoutSubItem &menu)
	{
		std::optional<bool> enabled;
		for (const auto item : menu.Items())
		{
			if (item.Tag().try_as<hstring>() == L"Enabled")
			{
				if (const auto toggleButton = item.try_as<wuxc::ToggleMenuFlyoutItem>())
				{
					enabled = toggleButton.IsChecked();
				}
			}
		}

		txmp::TaskbarAppearance appearance(nullptr);
		if (enabled)
		{
			txmp::OptionalTaskbarAppearance optAppearance;
			optAppearance.Enabled(*enabled);
			appearance = std::move(optAppearance);
		}
		else
		{
			appearance = { };
		}

		for (const auto item : menu.Items())
		{
			const auto tag = item.Tag();
			if (const auto radioItem = item.try_as<Controls::RadioMenuFlyoutItem>())
			{
				if (radioItem.IsChecked())
				{
					if (const auto accent = tag.try_as<txmp::AccentState>())
					{
						appearance.Accent(*accent);
					}
				}
			}
			else if (const auto toggleItem = item.try_as<wuxc::ToggleMenuFlyoutItem>())
			{
				if (tag.try_as<hstring>() == L"ShowPeek")
				{
					appearance.ShowPeek(toggleItem.IsChecked());
				}
			}
		}

		return appearance;
	}

	wuxc::MenuFlyoutSubItem TrayFlyoutPage::GetSubMenuForState(txmp::TaskbarState state)
	{
		for (const wuxc::MenuFlyoutItemBase item : ContextMenu().Items())
		{
			if (const auto submenu = item.try_as<wuxc::MenuFlyoutSubItem>())
			{
				const auto tag = submenu.Tag().try_as<txmp::TaskbarState>();
				if (tag == state)
				{
					return submenu;
				}
			}
		}

		return nullptr;
	}
}
