#pragma once
#include "../dependencyproperty.h"
#include "../event.h"
#include "../factory.h"
#include "winrt.hpp"

#include "Pages/FramelessPage.g.h"
#include "Controls/ChromeButton.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage>
	{
		FramelessPage();

		virtual bool CanMove() noexcept;

		void ShowSystemMenu(const wf::Point &position);
		void HideSystemMenu();

		virtual bool RequestClose();
		virtual wf::Rect DragRegion();
		wf::Rect TitlebarButtonsRegion();

		void Close();
		DECL_EVENT(ClosedDelegate, Closed, m_ClosedHandler);

		void CloseClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void SystemMenuOpening(const IInspectable &sender, const IInspectable &args);

		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(hstring, Title, box_value(L""));
		DECL_DEPENDENCY_PROPERTY(wux::UIElement, UserContent);
		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(bool, ExpandIntoTitlebar, box_value(false));
		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(bool, IsClosable, box_value(true));
		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(bool, AlwaysOnTop, box_value(false));

		wfc::IObservableVector<Controls::ChromeButton> TitlebarContent() noexcept
		{
			return m_TitlebarContent;
		}

		wfc::IObservableVector<wuxc::MenuFlyoutItemBase> SystemMenuContent() noexcept
		{
			return m_SystemMenuContent;
		}

		~FramelessPage();

	private:
		void SystemMenuChanged(const wfc::IObservableVector<wuxc::MenuFlyoutItemBase> &sender, const wfc::IVectorChangedEventArgs &event);

		wfc::IObservableVector<Controls::ChromeButton> m_TitlebarContent = single_threaded_observable_vector<Controls::ChromeButton>();

		bool m_NeedsSystemMenuRefresh = false;
		event_token m_SystemMenuChangedToken;
		wfc::IObservableVector<wuxc::MenuFlyoutItemBase> m_SystemMenuContent = single_threaded_observable_vector<wuxc::MenuFlyoutItemBase>();

		static wux::Style LookupStyle(const IInspectable &key);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, FramelessPage);
