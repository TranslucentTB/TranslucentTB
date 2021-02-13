#pragma once
#include "../event.h"
#include "../factory.h"
#include "winrt.hpp"

#include "FramelessPage.h"
#include "Pages/ColorPickerPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct ColorPickerPage : ColorPickerPageT<ColorPickerPage>
	{
		ColorPickerPage(const hstring &category, const Windows::UI::Color &currentColor);

		bool RequestClose() override;

		DECL_EVENT_FUNCS(ChangesCommittedDelegate, ChangesCommitted, m_ChangesCommittedHandler);

		void OkButtonClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void CancelButtonClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);
		void ApplyButtonClicked(const IInspectable &sender, const wux::RoutedEventArgs &args);

		void DialogOpened(const IInspectable &sender, const wuxc::ContentDialogOpenedEventArgs &args);
		void DialogClosed(const IInspectable &sender, const wuxc::ContentDialogClosedEventArgs &args);

	private:
		fire_and_forget OpenConfirmDialog();
		void DismissConfirmDialog();

		bool m_DialogOpened = false;
		event<ChangesCommittedDelegate> m_ChangesCommittedHandler;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, ColorPickerPage);
