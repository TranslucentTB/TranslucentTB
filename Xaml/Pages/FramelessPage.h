#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"

#include "Pages/FramelessPage.g.h"
#include "Controls/ChromeButton.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage>
	{
		FramelessPage();

		virtual bool CanMove() noexcept;

		void ShowSystemMenu(const Windows::Foundation::Point &position);
		void HideSystemMenu();

		virtual bool RequestClose();
		virtual Windows::Foundation::Rect DragRegion();

		void Close();
		event_token Closed(const ClosedDelegate &handler);
		void Closed(const event_token &token);

		void CloseClicked(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		DECL_DEPENDENCY_PROPERTY(hstring, Title);
		DECL_DEPENDENCY_PROPERTY(Windows::Foundation::Collections::IObservableVector<Controls::ChromeButton>, TitlebarContent);
		DECL_DEPENDENCY_PROPERTY(Windows::UI::Xaml::UIElement, UserContent);
		DECL_DEPENDENCY_PROPERTY(bool, ExpandIntoTitlebar);
		DECL_DEPENDENCY_PROPERTY(bool, IsClosable);
		DECL_DEPENDENCY_PROPERTY(bool, AlwaysOnTop);

	private:
		event<ClosedDelegate> m_ClosedHandler;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, FramelessPage);
