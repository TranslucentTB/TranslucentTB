#pragma once
#include "../event.h"
#include "../factory.h"
#include "../PropertyChangedBase.hpp"
#include "winrt.hpp"

#include "Models/Primitives/TaskbarAppearance.h"
#include "Pages/TrayFlyoutPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct TrayFlyoutPage : TrayFlyoutPageT<TrayFlyoutPage>, PropertyChangedBase<TrayFlyoutPage>
	{
		TrayFlyoutPage(bool hasPackageIdentity);

		bool IsBlurSupported() noexcept
		{
			return m_BlurSupported;
		}

		bool HasPackageIdentity() noexcept
		{
			return m_HasPackageIdentity;
		}

		DECL_EVENT(TaskbarSettingsChangedDelegate, TaskbarSettingsChanged, m_TaskbarSettingsChangedDelegate);
		DECL_EVENT(ColorRequestedDelegate, ColorRequested, m_ColorRequestedDelegate);

		DECL_EVENT(OpenLogFileRequestedDelegate, OpenLogFileRequested, m_OpenLogFileRequestedDelegate);
		DECL_EVENT(LogLevelChangedDelegate, LogLevelChanged, m_LogLevelChangedDelegate);
		DECL_EVENT(DumpDynamicStateRequestedDelegate, DumpDynamicStateRequested, m_DumpDynamicStateRequestedDelegate);
		DECL_EVENT(EditSettingsRequestedDelegate, EditSettingsRequested, m_EditSettingsRequestedDelegate);
		DECL_EVENT(ResetSettingsRequestedDelegate, ResetSettingsRequested, m_ResetSettingsRequestedDelegate);
		DECL_EVENT(DisableSavingSettingsChangedDelegate, DisableSavingSettingsChanged, m_DisableSavingSettingsChangedDelegate);
		DECL_EVENT(HideTrayRequestedDelegate, HideTrayRequested, m_HideTrayRequestedDelegate);
		DECL_EVENT(ResetDynamicStateRequestedDelegate, ResetDynamicStateRequested, m_ResetDynamicStateRequestedDelegate);
		DECL_EVENT(CompactThunkHeapRequestedDelegate, CompactThunkHeapRequested, m_CompactThunkHeapRequestedDelegate);

		DECL_EVENT(StartupStateChangedDelegate, StartupStateChanged, m_StartupStateChangedDelegate);
		DECL_EVENT(TipsAndTricksRequestedDelegate, TipsAndTricksRequested, m_TipsAndTricksRequestedDelegate);
		DECL_EVENT(AboutRequestedDelegate, AboutRequested, m_AboutRequestedDelegate);
		DECL_EVENT(ExitRequestedDelegate, ExitRequested, m_ExitRequestedDelegate);

		void SetTaskbarSettings(const txmp::TaskbarState &state, const txmp::TaskbarAppearance &appearance);
		void SetLogLevel(const txmp::LogLevel &level);
		void SetDisableSavingSettings(const bool &disabled);
		void SetStartupState(const wf::IReference<Windows::ApplicationModel::StartupTaskState> &state);

		DECL_PROPERTY_CHANGED_FUNCS(txmp::LogSinkState, SinkState, m_SinkState);

		void AppearanceClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void ColorClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);

		void OpenLogFileClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void LogLevelClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void DumpDynamicStateClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void EditSettingsClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void ResetSettingsClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void DisableSavingSettingsClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void HideTrayClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void ResetDynamicStateClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void CompactThunkHeapClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);

		void StartupClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void TipsAndTricksClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void AboutClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void ExitClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);

	private:
		static wuxc::MenuFlyoutSubItem GetContainingSubMenu(const wuxc::MenuFlyoutItemBase &item, const wuxc::MenuFlyoutSubItem &subItem);
		wuxc::MenuFlyoutSubItem GetItemParent(const wuxc::MenuFlyoutItemBase &item);

		static txmp::TaskbarAppearance BuildAppearanceFromSubMenu(const wuxc::MenuFlyoutSubItem &menu);
		wuxc::MenuFlyoutSubItem GetSubMenuForState(txmp::TaskbarState state);

		txmp::LogSinkState m_SinkState = txmp::LogSinkState::Failed;
		bool m_BlurSupported, m_HasPackageIdentity;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, TrayFlyoutPage);
