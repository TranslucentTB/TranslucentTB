#include "pch.h"

#include "Models.ActionItem.h"
#if __has_include("Models.ActionItem.g.cpp")
#include "Models.ActionItem.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	hstring ActionItem::Name()
	{
		return m_name;
	}

	void ActionItem::Name(const hstring &value)
	{
		compare_assign(m_name, value, PROP_NAME);
	}

	hstring ActionItem::Description()
	{
		return m_description;
	}

	void ActionItem::Description(const hstring &value)
	{
		compare_assign(m_description, value, PROP_NAME);
	}

	Windows::UI::Xaml::UIElement ActionItem::Icon()
	{
		return m_icon;
	}

	void ActionItem::Icon(const Windows::UI::Xaml::UIElement &value)
	{
		compare_assign(m_icon, value, PROP_NAME);
	}

	event_token ActionItem::Click(const Windows::UI::Xaml::RoutedEventHandler &value)
	{
		return m_click.add(value);
	}

	void ActionItem::Click(const event_token &token)
	{
		m_click.remove(token);
	}

	void ActionItem::ForwardClick(const Windows::Foundation::IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args)
	{
		m_click(sender, args);
	}
}
