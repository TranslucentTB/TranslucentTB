#pragma once
#include "../factory.h"

#include "FramelessPage.h"
#include "Pages/ColorPickerPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct ColorPickerPage : ColorPickerPageT<ColorPickerPage>
	{
		ColorPickerPage(const hstring &category, const Windows::UI::Color &currentColor);

		bool CanMove() noexcept override;
		bool RequestClose() override;

		event_token ChangesCommitted(const ChangesCommittedDelegate &handler);
		void ChangesCommitted(const event_token &token);

		void OkButtonClicked(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void CancelButtonClicked(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);
		void ApplyButtonClicked(const IInspectable &sender, const Windows::UI::Xaml::RoutedEventArgs &args);

		void DialogOpened(const IInspectable &sender, const Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs &args);
		void DialogClosed(const IInspectable &sender, const Windows::UI::Xaml::Controls::ContentDialogClosedEventArgs &args);

	private:
		fire_and_forget OpenConfirmDialog();
		void DismissConfirmDialog();

		bool m_DialogOpened = false;
		event<ChangesCommittedDelegate> m_ChangesCommittedHandler;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, ColorPickerPage);
