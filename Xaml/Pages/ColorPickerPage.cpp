#include "pch.h"

#include "ColorPickerPage.h"
#if __has_include("Pages/ColorPickerPage.g.cpp")
#include "Pages/ColorPickerPage.g.cpp"
#endif

#include "appinfo.hpp"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	ColorPickerPage::ColorPickerPage(txmp::TaskbarState state, Windows::UI::Color originalColor) : m_State(state), m_OriginalColor(originalColor)
	{ }

	void ColorPickerPage::InitializeComponent()
	{
		ComponentConnectorT::InitializeComponent();

		const auto resourceLoader = Windows::ApplicationModel::Resources::ResourceLoader::GetForUIContext(UIContext());
		Title(winrt::format(L"{} - {} - " APP_NAME,
			resourceLoader.GetString(GetResourceForState(m_State)),
			resourceLoader.GetString(L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_AccentColor/Text")));
	}

	bool ColorPickerPage::CanMove() noexcept
	{
		return !m_DialogOpened;
	}

	bool ColorPickerPage::Close()
	{
		if (m_DialogOpened)
		{
			return false;
		}

		if (Picker().Color() != m_OriginalColor)
		{
			OpenConfirmDialog();
			return false;
		}
		else
		{
			return base_type::Close();
		}
	}

	void ColorPickerPage::OkButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		m_ChangesCommittedHandler(Picker().Color());
		base_type::Close();
	}

	void ColorPickerPage::CancelButtonClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		base_type::Close();
	}

	void ColorPickerPage::DialogOpened(const IInspectable &, const wuxc::ContentDialogOpenedEventArgs &) noexcept
	{
		m_DialogOpened = true;
	}

	void ColorPickerPage::DialogClosed(const IInspectable &, const wuxc::ContentDialogClosedEventArgs &) noexcept
	{
		m_DialogOpened = false;
	}

	Windows::UI::Color ColorPickerPage::OriginalColor() noexcept
	{
		return m_OriginalColor;
	}

	void ColorPickerPage::PickerColorChanged(const muxc::ColorPicker &, const muxc::ColorChangedEventArgs &args)
	{
		m_ColorChangedHandler(args.NewColor());
	}

	std::wstring_view ColorPickerPage::GetResourceForState(txmp::TaskbarState state)
	{
		switch (state)
		{
			using enum txmp::TaskbarState;
		case Desktop: return L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_Desktop/Text";
		case VisibleWindow: return L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_VisibleWindow/Text";
		case MaximisedWindow: return L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_MaximizedWindow/Text";
		case StartOpened: return L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_StartOpened/Text";
		case SearchOpened: return L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_SearchOpened/Text";
		case TaskViewOpened: return L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_TaskViewOpened/Text";
		case BatterySaver: return L"/TranslucentTB.Xaml/Resources/TrayFlyoutPage_BatterySaver/Text";
		default: throw std::invalid_argument("Unknown taskbar state");
		}
	}

	fire_and_forget ColorPickerPage::OpenConfirmDialog()
	{
		const auto self_weak = get_weak();
		const auto result = co_await ConfirmCloseDialog().ShowAsync();

		if (const auto self = self_weak.get())
		{
			if (result != wuxc::ContentDialogResult::None)
			{
				if (result == wuxc::ContentDialogResult::Primary)
				{
					self->m_ChangesCommittedHandler(self->Picker().Color());
				}

				self->base_type::Close();
			}
		}
	}
}
