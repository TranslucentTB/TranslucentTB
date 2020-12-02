#pragma once
#include "../factory.h"

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

FACTORY(winrt::TranslucentTB::Xaml::Controls, ActionList);
