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

		DECL_DEPENDENCY_PROPERTY(hstring, Title);
		DECL_DEPENDENCY_PROPERTY(wfc::IObservableVector<Controls::ChromeButton>, TitlebarContent);
		DECL_DEPENDENCY_PROPERTY(wux::UIElement, UserContent);
		DECL_DEPENDENCY_PROPERTY(bool, ExpandIntoTitlebar);
		DECL_DEPENDENCY_PROPERTY(bool, IsClosable);
		DECL_DEPENDENCY_PROPERTY(bool, AlwaysOnTop);

	private:
		event<ClosedDelegate> m_ClosedHandler;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, FramelessPage);
