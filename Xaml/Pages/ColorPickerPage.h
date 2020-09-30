#pragma once

#include "FramelessPageWithMessageDialog.h"
#include "Pages/ColorPickerPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct ColorPickerPage : ColorPickerPageT<ColorPickerPage>
	{
		ColorPickerPage(const hstring &category, const Windows::UI::Color &currentColor);

		bool RequestClose() override;

	private:
		fire_and_forget OpenConfirmDialog();
		bool m_DialogOpened = false;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct ColorPickerPage : ColorPickerPageT<ColorPickerPage, implementation::ColorPickerPage>
	{
	};
}
