#pragma once
#include "../event.h"
#include "../factory.h"
#include "../PropertyChangedBase.hpp"
#include "winrt.hpp"

#include "FramelessPage.h"
#include "Pages/ColorPickerPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct ColorPickerPage : ColorPickerPageT<ColorPickerPage>
	{
		ColorPickerPage(txmp::TaskbarState state, const Windows::UI::Color &currentColor);

		bool CanMove() noexcept override;
		bool RequestClose() override;

		DECL_EVENT(ColorChangedDelegate, ColorChanged, m_ColorChangedHandler);
		DECL_EVENT(ChangesCommittedDelegate, ChangesCommitted, m_ChangesCommittedHandler);

		void OkButtonClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void CancelButtonClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);

		void DialogOpened(const IInspectable &sender, const wuxc::ContentDialogOpenedEventArgs &args) noexcept;
		void DialogClosed(const IInspectable &sender, const wuxc::ContentDialogClosedEventArgs &args) noexcept;

		void PickerColorChanged(const muxc::ColorPicker &sender, const muxc::ColorChangedEventArgs &args);

	private:
		static std::wstring_view GetTextForState(txmp::TaskbarState state);
		fire_and_forget OpenConfirmDialog();

		bool m_DialogOpened = false;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, ColorPickerPage);
