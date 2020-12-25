#pragma once
#include "../dependencyproperty.h"
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
		event_token Closed(const ClosedDelegate &handler);
		void Closed(const event_token &token);

		void CloseClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);

		DECL_DEPENDENCY_PROPERTY(hstring, Title);
		DECL_DEPENDENCY_PROPERTY(wf::Collections::IObservableVector<Controls::ChromeButton>, TitlebarContent);
		DECL_DEPENDENCY_PROPERTY(wux::UIElement, UserContent);
		DECL_DEPENDENCY_PROPERTY(bool, ExpandIntoTitlebar);
		DECL_DEPENDENCY_PROPERTY(bool, IsClosable);
		DECL_DEPENDENCY_PROPERTY(bool, AlwaysOnTop);

	private:
		event<ClosedDelegate> m_ClosedHandler;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, FramelessPage);
