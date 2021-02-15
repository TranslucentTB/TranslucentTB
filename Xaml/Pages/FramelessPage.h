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

		bool CanMove();

		void ShowSystemMenu(const wf::Point &position);
		void HideSystemMenu();

		virtual bool RequestClose();
		virtual wf::Rect DragRegion();
		wf::Rect TitlebarButtonsRegion();

		void Close();
		DECL_EVENT_FUNCS(ClosedDelegate, Closed, m_ClosedHandler);

		void CloseClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void SystemMenuOpening(const IInspectable &sender, const IInspectable &args);

		DECL_DEPENDENCY_PROPERTY(hstring, Title);
		DECL_DEPENDENCY_PROPERTY(wfc::IObservableVector<Controls::ChromeButton>, TitlebarContent);
		DECL_DEPENDENCY_PROPERTY_FUNCS(wfc::IObservableVector<wuxc::MenuFlyoutItemBase>, SystemMenuContent, s_SystemMenuContentProperty);
		DECL_DEPENDENCY_PROPERTY(wux::UIElement, UserContent);
		DECL_DEPENDENCY_PROPERTY(bool, ExpandIntoTitlebar);
		DECL_DEPENDENCY_PROPERTY(bool, IsClosable);
		DECL_DEPENDENCY_PROPERTY(bool, AlwaysOnTop);

	private:
		void SystemMenuChanged(const wfc::IObservableVector<wuxc::MenuFlyoutItemBase> &sender, const wfc::IVectorChangedEventArgs &event);

		event<ClosedDelegate> m_ClosedHandler;
		bool m_NeedsSystemMenuRefresh = true;
		wfc::IObservableVector<wuxc::MenuFlyoutItemBase>::VectorChanged_revoker m_SystemMenuChangedRevoker;

		static wux::Style LookupStyle(const IInspectable &key);
		static void SystemMenuContentChanged(const Windows::UI::Xaml::DependencyObject &d, const Windows::UI::Xaml::DependencyPropertyChangedEventArgs &e);
		static wux::DependencyProperty s_SystemMenuContentProperty;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, FramelessPage);
