#include "pch.h"

#include "Pages.FramelessPage.h"
#if __has_include("Pages.FramelessPage.g.cpp")
#include "Pages.FramelessPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Pages::implementation
{
	DependencyProperty FramelessPage::s_TitleProperty =
		DependencyProperty::Register(
			L"Title",
			xaml_typename<hstring>(),
			xaml_typename<TranslucentTB::Pages::FramelessPage>(),
			{ box_value(L"") }
		);

	DependencyProperty FramelessPage::s_UserContentProperty =
		DependencyProperty::Register(
			L"UserContent",
			xaml_typename<UIElement>(),
			xaml_typename<TranslucentTB::Pages::FramelessPage>(),
			{ nullptr }
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

	DependencyProperty FramelessPage::TitleProperty()
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

	DependencyProperty FramelessPage::UserContentProperty()
	{
		return s_UserContentProperty;
	}
}
