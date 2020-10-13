#include "pch.h"

#include "FramelessPage.h"
#if __has_include("Pages/FramelessPage.g.cpp")
#include "Pages/FramelessPage.g.cpp"
#endif

#include "util/strings.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	DependencyProperty FramelessPage::s_TitleProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(Title),
			xaml_typename<hstring>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty FramelessPage::s_TitlebarContentProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(TitlebarContent),
			xaml_typename<Windows::Foundation::Collections::IObservableVector<Controls::ChromeButton>>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty FramelessPage::s_UserContentProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(UserContent),
			xaml_typename<UIElement>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty FramelessPage::s_IsClosableProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(IsClosable),
			xaml_typename<bool>(),
			xaml_typename<class_type>(),
			nullptr
	);

	DependencyProperty FramelessPage::s_AlwaysOnTopProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(AlwaysOnTop),
			xaml_typename<bool>(),
			xaml_typename<class_type>(),
			nullptr
	);

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

	hstring FramelessPage::Title()
	{
		return unbox_value<hstring>(GetValue(s_TitleProperty));
	}

	void FramelessPage::Title(const hstring &title)
	{
		SetValue(s_TitleProperty, box_value(title));
	}

	DependencyProperty FramelessPage::TitleProperty() noexcept
	{
		return s_TitleProperty;
	}

	Windows::Foundation::Collections::IObservableVector<Controls::ChromeButton> FramelessPage::TitlebarContent()
	{
		return GetValue(s_TitlebarContentProperty).as<Windows::Foundation::Collections::IObservableVector<Controls::ChromeButton>>();
	}

	void FramelessPage::TitlebarContent(const Windows::Foundation::Collections::IObservableVector<Controls::ChromeButton> &content)
	{
		SetValue(s_TitlebarContentProperty, content);
	}

	Windows::UI::Xaml::DependencyProperty FramelessPage::TitlebarContentProperty() noexcept
	{
		return s_TitlebarContentProperty;
	}

	UIElement FramelessPage::UserContent()
	{
		return GetValue(s_UserContentProperty).as<UIElement>();
	}

	void FramelessPage::UserContent(const UIElement &element)
	{
		SetValue(s_UserContentProperty, element);
	}

	DependencyProperty FramelessPage::UserContentProperty() noexcept
	{
		return s_UserContentProperty;
	}

	bool FramelessPage::IsClosable()
	{
		return unbox_value<bool>(GetValue(s_IsClosableProperty));
	}

	void FramelessPage::IsClosable(bool closeable)
	{
		SetValue(s_IsClosableProperty, box_value(closeable));
	}

	DependencyProperty FramelessPage::IsClosableProperty() noexcept
	{
		return s_IsClosableProperty;
	}

	bool FramelessPage::AlwaysOnTop()
	{
		return unbox_value<bool>(GetValue(s_AlwaysOnTopProperty));
	}

	void FramelessPage::AlwaysOnTop(bool alwaysOnTop)
	{
		SetValue(s_AlwaysOnTopProperty, box_value(alwaysOnTop));
	}

	DependencyProperty FramelessPage::AlwaysOnTopProperty() noexcept
	{
		return s_AlwaysOnTopProperty;
	}
}
