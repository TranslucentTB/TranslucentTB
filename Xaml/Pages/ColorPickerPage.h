#pragma once
#include <Shobjidl.h>

#include "FramelessPage.h"
#include "Pages/ColorPickerPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct ColorPickerPage : ColorPickerPageT<ColorPickerPage, IInitializeWithWindow>
	{
		ColorPickerPage(const winrt::hstring &category, const Windows::UI::Color &currentColor);

		bool RequestClose() override;

		IFACEMETHOD(Initialize)(HWND hwnd) noexcept override;

	private:
		fire_and_forget OpenConfirmDialog();

		Windows::UI::Popups::MessageDialog m_Dialog;
		bool m_DialogOpened = false;
	};
}

namespace winrt::TranslucentTB::Xaml::Pages::factory_implementation
{
	struct ColorPickerPage : ColorPickerPageT<ColorPickerPage, implementation::ColorPickerPage>
	{
	};
}

// i hate this
// https://github.com/microsoft/microsoft-ui-xaml/issues/3331
#define ColorPickerPage ColorPickerPage, IInitializeWithWindow
