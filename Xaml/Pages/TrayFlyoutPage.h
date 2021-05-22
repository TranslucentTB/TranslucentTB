#pragma once
#include "../event.h"
#include "../factory.h"
#include "../PropertyChangedBase.hpp"
#include "winrt.hpp"

#include "FlyoutPage.h"
#include "Models/Primitives/TaskbarAppearance.h"
#include "Pages/TrayFlyoutPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct TrayFlyoutPage : TrayFlyoutPageT<TrayFlyoutPage>, PropertyChangedBase<TrayFlyoutPage>
	{
		TrayFlyoutPage(bool hasPackageIdentity);

		DECL_EVENT_FUNCS(TaskbarSettingsChangedDelegate, TaskbarSettingsChanged, m_TaskbarSettingsChangedDelegate);
		DECL_EVENT_FUNCS(ColorRequestedDelegate, ColorRequested, m_ColorRequestedDelegate);

		DECL_EVENT_FUNCS(OpenLogFileRequestedDelegate, OpenLogFileRequested, m_OpenLogFileRequestedDelegate);
		DECL_EVENT_FUNCS(LogLevelChangedDelegate, LogLevelChanged, m_LogLevelChangedDelegate);
		DECL_EVENT_FUNCS(DumpDynamicStateRequestedDelegate, DumpDynamicStateRequested, m_DumpDynamicStateRequestedDelegate);
		DECL_EVENT_FUNCS(EditSettingsRequestedDelegate, EditSettingsRequested, m_EditSettingsRequestedDelegate);
		DECL_EVENT_FUNCS(ResetSettingsRequestedDelegate, ResetSettingsRequested, m_ResetSettingsRequestedDelegate);
		DECL_EVENT_FUNCS(DisableSavingSettingsChangedDelegate, DisableSavingSettingsChanged, m_DisableSavingSettingsChangedDelegate);
		DECL_EVENT_FUNCS(HideTrayRequestedDelegate, HideTrayRequested, m_HideTrayRequestedDelegate);
		DECL_EVENT_FUNCS(ResetDynamicStateRequestedDelegate, ResetDynamicStateRequested, m_ResetDynamicStateRequestedDelegate);
		DECL_EVENT_FUNCS(CompactThunkHeapRequestedDelegate, CompactThunkHeapRequested, m_CompactThunkHeapRequestedDelegate);

		DECL_EVENT_FUNCS(StartupStateChangedDelegate, StartupStateChanged, m_StartupStateChangedDelegate);
		DECL_EVENT_FUNCS(TipsAndTricksRequestedDelegate, TipsAndTricksRequested, m_TipsAndTricksRequestedDelegate);
		DECL_EVENT_FUNCS(AboutRequestedDelegate, AboutRequested, m_AboutRequestedDelegate);
		DECL_EVENT_FUNCS(ExitRequestedDelegate, ExitRequested, m_ExitRequestedDelegate);

		void SetTaskbarSettings(const Models::Primitives::TaskbarState &state, const Models::Primitives::TaskbarAppearance &appearance);
		void SetLogLevel(const Models::Primitives::LogLevel &level);
		void SetDisableSavingSettings(const bool &disabled);
		void SetStartupState(const wf::IReference<Windows::ApplicationModel::StartupTaskState> &state);

		DECL_PROPERTY_CHANGED_FUNCS(Models::Primitives::LogSinkState, SinkState, m_SinkState);

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

		static Models::Primitives::TaskbarAppearance BuildAppearanceFromSubMenu(const wuxc::MenuFlyoutSubItem &menu);
		wuxc::MenuFlyoutSubItem GetSubMenuForState(Models::Primitives::TaskbarState state);

		event<TaskbarSettingsChangedDelegate> m_TaskbarSettingsChangedDelegate;
		event<ColorRequestedDelegate> m_ColorRequestedDelegate;

		event<OpenLogFileRequestedDelegate> m_OpenLogFileRequestedDelegate;
		event<LogLevelChangedDelegate> m_LogLevelChangedDelegate;
		event<DumpDynamicStateRequestedDelegate> m_DumpDynamicStateRequestedDelegate;
		event<EditSettingsRequestedDelegate> m_EditSettingsRequestedDelegate;
		event<ResetSettingsRequestedDelegate> m_ResetSettingsRequestedDelegate;
		event<DisableSavingSettingsChangedDelegate> m_DisableSavingSettingsChangedDelegate;
		event<HideTrayRequestedDelegate> m_HideTrayRequestedDelegate;
		event<ResetDynamicStateRequestedDelegate> m_ResetDynamicStateRequestedDelegate;
		event<CompactThunkHeapRequestedDelegate> m_CompactThunkHeapRequestedDelegate;

		event<StartupStateChangedDelegate> m_StartupStateChangedDelegate;
		event<TipsAndTricksRequestedDelegate> m_TipsAndTricksRequestedDelegate;
		event<AboutRequestedDelegate> m_AboutRequestedDelegate;
		event<ExitRequestedDelegate> m_ExitRequestedDelegate;

		Models::Primitives::LogSinkState m_SinkState = Models::Primitives::LogSinkState::Failed;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, TrayFlyoutPage);
