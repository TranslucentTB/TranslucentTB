#pragma once

#include "Pages/FramelessPage.g.h"
#include "Controls/ChromeButton.h"
#include "..\dependencyproperty.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage>
	{
		FramelessPage();

		virtual bool RequestClose();
		virtual Windows::Foundation::Rect DragRegion();

		void Close();
		event_token Closed(const ClosedDelegate &handler);
		void Closed(const event_token &token);

		void CloseButtonClicked(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		DECL_VALUE_DEPENDENCY_PROPERTY(hstring, Title);
		DECL_REF_DEPENDENCY_PROPERTY(Windows::Foundation::Collections::IObservableVector<Controls::ChromeButton>, TitlebarContent);
		DECL_REF_DEPENDENCY_PROPERTY(Windows::UI::Xaml::UIElement, UserContent);
		DECL_VALUE_DEPENDENCY_PROPERTY(bool, ExpandIntoTitlebar);
		DECL_VALUE_DEPENDENCY_PROPERTY(bool, IsClosable);
		DECL_VALUE_DEPENDENCY_PROPERTY(bool, AlwaysOnTop);

	private:
		event<ClosedDelegate> m_ClosedHandler;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct FramelessPage : FramelessPageT<FramelessPage, implementation::FramelessPage>
	{
	};
}
