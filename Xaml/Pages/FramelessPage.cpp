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

	bool FramelessPage::CanMove() noexcept
	{
		return true;
	}

	void FramelessPage::ShowSystemMenu(const Windows::Foundation::Point &position)
	{
		if (CanMove())
		{
			SystemMenu().ShowAt(*this, position);
		}
	}

	void FramelessPage::HideSystemMenu()
	{
		SystemMenu().Hide();
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

	Windows::Foundation::Rect FramelessPage::DragRegion()
	{
		if (!ExpandIntoTitlebar())
		{
			return {
				0,
				0,
				static_cast<float>(ActualWidth() - (CloseButton().ActualWidth() + CustomTitlebarControls().ActualWidth())),
				static_cast<float>(Titlebar().ActualHeight())
			};
		}
		else
		{
			throw hresult_not_implemented(L"A page that uses ExpandIntoTitlebar should override DragRegion.");
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

	void FramelessPage::CloseClicked(const IInspectable &, const RoutedEventArgs &)
	{
		RequestClose();
	}
}
