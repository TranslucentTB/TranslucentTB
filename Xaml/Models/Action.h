#pragma once
#include "../factory.h"
#include "../PropertyChangedBase.hpp"
#include "winrt.hpp"

#include "Models/Action.g.h"

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	struct Action : ActionT<Action>, PropertyChangedBase<Action>
	{
		Action() = default;

		hstring Name();
		void Name(const hstring &value);

		hstring Description();
		void Description(const hstring &value);

		wuxc::IconElement Icon();
		void Icon(const wuxc::IconElement &value);

		event_token Click(const wux::RoutedEventHandler &value);
		void Click(const event_token &token);

		void ForwardClick(const IInspectable &sender, const wux::RoutedEventArgs &args);

	private:
		hstring m_name;
		hstring m_description;
		wuxc::IconElement m_icon = nullptr;
		event<wux::RoutedEventHandler> m_click;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Models, Action);
