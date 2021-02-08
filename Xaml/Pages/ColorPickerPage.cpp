#include "pch.h"

#include "ColorPickerPage.h"
#if __has_include("Pages/ColorPickerPage.g.cpp")
#include "Pages/ColorPickerPage.g.cpp"
#endif

#include "appinfo.hpp"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	ColorPickerPage::ColorPickerPage(const hstring &category, const Windows::UI::Color &currentColor)
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

	event_token ColorPickerPage::ChangesCommitted(const ChangesCommittedDelegate &handler)
	{
		return m_ChangesCommittedHandler.add(handler);
	}

	void ColorPickerPage::ChangesCommitted(const event_token &token)
	{
		m_ChangesCommittedHandler.remove(token);
	}

	void ColorPickerPage::OkButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		// A bug in ContentDialog allows you to click the OK and Cancel button
		// while it's opened. So if we don't cancel it, and the thread is reused,
		// the ContentDialog is still considered as "opened", and will throw if we
		// attempt to open another one.
		DismissConfirmDialog();

		m_ChangesCommittedHandler(Picker().Color());
		Close();
	}

	void ColorPickerPage::CancelButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		DismissConfirmDialog();
		Close();
	}

	void ColorPickerPage::ApplyButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		DismissConfirmDialog();

		m_ChangesCommittedHandler(Picker().Color());
		Picker().PreviousColor(Picker().Color());
	}

	void ColorPickerPage::DialogOpened(const IInspectable &, const wuxc::ContentDialogOpenedEventArgs &)
	{
		m_DialogOpened = true;
	}

	void ColorPickerPage::DialogClosed(const IInspectable &, const wuxc::ContentDialogClosedEventArgs &)
	{
		m_DialogOpened = false;
	}

	fire_and_forget ColorPickerPage::OpenConfirmDialog()
	{
		const auto self_weak = get_weak();
		const auto result = co_await Dialog().ShowAsync();

		if (const auto self = self_weak.get())
		{
			if (result != wuxc::ContentDialogResult::None)
			{
				if (result == wuxc::ContentDialogResult::Primary)
				{
					self->m_ChangesCommittedHandler(self->Picker().Color());
				}

				self->Close();
			}
		}
	}

	void ColorPickerPage::DismissConfirmDialog()
	{
		if (m_DialogOpened)
		{
			Dialog().Hide();
		}
	}
}
