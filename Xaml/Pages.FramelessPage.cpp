#include "pch.h"

#include "util/strings.hpp"

#include "Pages.FramelessPage.h"
#if __has_include("Pages.FramelessPage.g.cpp")
#include "Pages.FramelessPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	DependencyProperty FramelessPage::s_TitleProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(Title),
			xaml_typename<hstring>(),
			xaml_typename<class_type>(),
			PropertyMetadata { box_value(L"") }
		);

	DependencyProperty FramelessPage::s_UserContentProperty =
		DependencyProperty::Register(
			UTIL_STRINGIFY(UserContent),
			xaml_typename<UIElement>(),
			xaml_typename<class_type>(),
			nullptr
	);

	FramelessPage::FramelessPage()
	{
		InitializeComponent();
	}

	hstring FramelessPage::Title()
	{
		return unbox_value<hstring>(GetValue(s_TitleProperty));
	}

	void FramelessPage::Title(hstring title)
	{
		SetValue(s_TitleProperty, box_value(title));
	}

	DependencyProperty FramelessPage::TitleProperty() noexcept
	{
		return s_TitleProperty;
	}

	UIElement FramelessPage::UserContent()
	{
		return GetValue(s_UserContentProperty).as<UIElement>();
	}

	void FramelessPage::UserContent(UIElement element)
	{
		SetValue(s_UserContentProperty, element);
	}

	DependencyProperty FramelessPage::UserContentProperty() noexcept
	{
		return s_UserContentProperty;
	}
}
