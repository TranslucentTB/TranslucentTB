#include "pch.h"

#include "ColorPickerPage.h"
#undef ColorPickerPage
#if __has_include("Pages/ColorPickerPage.g.cpp")
#include "Pages/ColorPickerPage.g.cpp"
#endif

#include "appinfo.hpp"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	ColorPickerPage::ColorPickerPage(const winrt::hstring &category, const Windows::UI::Color &currentColor) :
		m_Dialog(L"Do you want to save changes to the color?", APP_NAME)
	{
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

	IFACEMETHODIMP ColorPickerPage::Initialize(HWND hwnd) noexcept
	{
		com_ptr<IInitializeWithWindow> initWithWindow;
		HRESULT hr = winrt::get_unknown(m_Dialog)->QueryInterface(initWithWindow.put());
		if (SUCCEEDED(hr))
		{
			hr = initWithWindow->Initialize(hwnd);
		}

		return hr;
	}

	fire_and_forget ColorPickerPage::OpenConfirmDialog()
	{
		m_DialogOpened = true;
		co_await m_Dialog.ShowAsync();
		m_DialogOpened = false;
	}
}
