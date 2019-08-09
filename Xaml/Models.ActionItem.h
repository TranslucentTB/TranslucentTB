#pragma once

#include "Models.ActionItem.g.h"

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	struct ActionItem : ActionItemT<ActionItem>
	{
		ActionItem() = default;

		hstring Name();
		void Name(const hstring &value);

		hstring Description();
		void Description(const hstring &value);

		hstring Icon();
		void Icon(const hstring &value);

		event_token PropertyChanged(const Windows::UI::Xaml::Data::PropertyChangedEventHandler &value);
		void PropertyChanged(const event_token &token);

		event_token Click(const Windows::UI::Xaml::RoutedEventHandler &value);
		void Click(const event_token &token);

		void ForwardClick(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

	private:
		template<typename T>
		void compare_assign(T &value, const T &new_value, hstring name)
		{
			if (value != new_value)
			{
				value = new_value;
				m_propertyChanged(*this, Data::PropertyChangedEventArgs(name));
			}
		}

		hstring m_name;
		hstring m_description;
		hstring m_icon;
		event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
		event<Windows::UI::Xaml::RoutedEventHandler> m_click;
	};
}

namespace winrt::TranslucentTB::Xaml::Models::factory_implementation
{
	struct ActionItem : ActionItemT<ActionItem, implementation::ActionItem>
	{
	};
}
