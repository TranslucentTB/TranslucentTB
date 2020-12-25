#include "pch.h"

#include "Action.h"
#if __has_include("Models/Action.g.cpp")
#include "Models/Action.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	hstring Action::Name()
	{
		return m_name;
	}

	void Action::Name(const hstring &value)
	{
		compare_assign(m_name, value, PROP_NAME);
	}

	hstring Action::Description()
	{
		return m_description;
	}

	void Action::Description(const hstring &value)
	{
		compare_assign(m_description, value, PROP_NAME);
	}

	wuxc::IconElement Action::Icon()
	{
		return m_icon;
	}

	void Action::Icon(const wuxc::IconElement &value)
	{
		compare_assign(m_icon, value, PROP_NAME);
	}

	event_token Action::Click(const wux::RoutedEventHandler &value)
	{
		return m_click.add(value);
	}

	void Action::Click(const event_token &token)
	{
		m_click.remove(token);
	}

	void Action::ForwardClick(const IInspectable &sender, const wux::RoutedEventArgs &args)
	{
		m_click(sender, args);
	}
}
