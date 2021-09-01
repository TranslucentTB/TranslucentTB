#include "pch.h"

#include "Action.h"
#if __has_include("Models/Action.g.cpp")
#include "Models/Action.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	void Action::ForwardClick(const IInspectable &sender, const wux::RoutedEventArgs &args)
	{
		m_click(sender, args);
	}
}
