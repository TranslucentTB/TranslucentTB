#pragma once
#include "../factory.h"
#include "winrt.hpp"

#include "Controls/ActionList.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct ActionList : ActionListT<ActionList>
	{
		void ForwardActionKey(const IInspectable &sender, const wux::Input::KeyRoutedEventArgs &args);
		void ForwardAction(const IInspectable &sender, const wux::RoutedEventArgs &args);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, ActionList);
