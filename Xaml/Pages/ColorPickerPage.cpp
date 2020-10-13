#include "pch.h"

#include "ColorPickerPage.h"
#if __has_include("Pages/ColorPickerPage.g.cpp")
#include "Pages/ColorPickerPage.g.cpp"
#endif

#include "appinfo.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	ColorPickerPage::ColorPickerPage(const hstring &category, const Windows::UI::Color &currentColor) :
		ColorPickerPageT<ColorPickerPage>(L"Do you want to save changes to the color?")
	{
		TitlebarContent(single_threaded_observable_vector<Controls::ChromeButton>());
		InitializeComponent();

		Title(category + L" - Color picker - " APP_NAME);
		Picker().PreviousColor(currentColor);
		Picker().Color(currentColor);
	}

	bool ColorPickerPage::RequestClose()
	{
		if (m_DialogOpened)
		{
			return false;
		}

		const auto prevCol = Picker().PreviousColor();
		if (!prevCol || Picker().Color() != prevCol.Value())
		{
			OpenConfirmDialog();
			return false;
		}
		else
		{
			Close();
			return true;
		}
	}

	fire_and_forget ColorPickerPage::OpenConfirmDialog()
	{
		// TODO: more complete impl
		m_DialogOpened = true;
		co_await Dialog().ShowAsync();
		m_DialogOpened = false;
	}
}
