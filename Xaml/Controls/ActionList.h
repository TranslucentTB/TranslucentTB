#pragma once

#include "Controls/ActionList.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct ActionList : ActionListT<ActionList>
	{
		ActionList();

		void ForwardActionKeyDown(const IInspectable &sender, const Windows::UI::Xaml::Input::KeyRoutedEventArgs &args);
		void ForwardAction(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
	};
}

namespace winrt::TranslucentTB::Xaml::Controls::factory_implementation
{
	struct ActionList : ActionListT<ActionList, implementation::ActionList>
	{
	};
}
