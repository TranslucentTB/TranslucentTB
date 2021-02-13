#pragma once
#include "../event.h"
#include "../factory.h"
#include "../PropertyChangedBase.hpp"
#include "winrt.hpp"

#include "Models/Action.g.h"

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	struct Action : ActionT<Action>, PropertyChangedBase<Action>
	{
		Action() = default;

		DECL_PROPERTY_CHANGED_PROP(hstring, Name);
		DECL_PROPERTY_CHANGED_PROP(hstring, Description);
		DECL_PROPERTY_CHANGED_FUNCS(wuxc::IconElement, Icon, m_icon);

		DECL_EVENT_FUNCS(wux::RoutedEventHandler, Click, m_click);

		void ForwardClick(const IInspectable &sender, const wux::RoutedEventArgs &args);

	private:
		wuxc::IconElement m_icon = nullptr;
		event<wux::RoutedEventHandler> m_click;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Models, Action);
