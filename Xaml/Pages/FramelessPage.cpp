#include "pch.h"

#include "FramelessPage.h"
#if __has_include("Pages/FramelessPage.g.cpp")
#include "Pages/FramelessPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	FramelessPage::FramelessPage()
	{
		InitializeComponent();
	}

	bool FramelessPage::RequestClose()
	{
		if (IsClosable())
		{
			Close();
			return true;
		}
		else
		{
			return false;
		}
	}

	void FramelessPage::Close()
	{
		m_ClosedHandler();
	}

	event_token FramelessPage::Closed(const ClosedDelegate &handler)
	{
		return m_ClosedHandler.add(handler);
	}

	void FramelessPage::Closed(const event_token &token)
	{
		m_ClosedHandler.remove(token);
	}

	void FramelessPage::CloseButtonClicked(const Windows::Foundation::IInspectable &, const Windows::UI::Xaml::RoutedEventArgs &)
	{
		RequestClose();
	}
}
