#include "pch.h"

#include "Controls/ActionList.h"
#if __has_include("Controls/ActionList.g.cpp")
#include "Controls/ActionList.g.cpp"
#endif

#include "../Models/Action.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	void ActionList::ForwardActionKey(const IInspectable &sender, const wux::Input::KeyRoutedEventArgs &args)
	{
		using enum Windows::System::VirtualKey;

		if (args.Key() == Enter || args.Key() == Space)
		{
			ForwardAction(sender, args);
			args.Handled(true);
		}
	}

	void ActionList::ForwardAction(const IInspectable &sender, const wux::RoutedEventArgs &args)
	{
		sender.as<wux::FrameworkElement>().Tag().as<Models::implementation::Action>()->ForwardClick(sender, args);
	}
}
