#pragma once

#include "Models.ActionItem.g.h"
#include "PropertyChangedBase.hpp"

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	struct ActionItem : ActionItemT<ActionItem>, PropertyChangedBase<ActionItem>
	{
		ActionItem() = default;

		hstring Name();
		void Name(const hstring &value);

		hstring Description();
		void Description(const hstring &value);

		hstring Icon();
		void Icon(const hstring &value);

		event_token Click(const Windows::UI::Xaml::RoutedEventHandler &value);
		void Click(const event_token &token);

		void ForwardClick(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

	private:
		hstring m_name;
		hstring m_description;
		hstring m_icon;
		event<Windows::UI::Xaml::RoutedEventHandler> m_click;
	};
}

namespace winrt::TranslucentTB::Xaml::Models::factory_implementation
{
	struct ActionItem : ActionItemT<ActionItem, implementation::ActionItem>
	{
	};
}
