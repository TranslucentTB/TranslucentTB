#include "pch.h"

#include "FramelessPage.h"
#if __has_include("Pages/FramelessPage.g.cpp")
#include "Pages/FramelessPage.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	FramelessPage::FramelessPage()
	{
		InitializeComponent();
	}

	bool FramelessPage::CanMove()
	{
		// block moving if a ContentDialog is opened
		for (const auto popup : wux::Media::VisualTreeHelper::GetOpenPopupsForXamlRoot(XamlRoot()))
		{
			if (popup.Child().try_as<wuxc::ContentDialog>())
			{
				return false;
			}
		}

		return true;
	}

	void FramelessPage::ShowSystemMenu(const wf::Point &position)
	{
		if (CanMove())
		{
			SystemMenu().ShowAt(*this, position);
		}
	}

	void FramelessPage::HideSystemMenu()
	{
		SystemMenu().Hide();
	}

	bool FramelessPage::RequestClose()
	{
		if (IsClosable())
		{
			Close();
			return true;
		}
		else
		{
			return false;
		}
	}

	wf::Rect FramelessPage::DragRegion()
	{
		if (!ExpandIntoTitlebar())
		{
			return {
				0.0f,
				0.0f,
				static_cast<float>(ActualWidth() - (CloseButton().ActualWidth() + CustomTitlebarControls().ActualWidth())),
				static_cast<float>(Titlebar().ActualHeight())
			};
		}
		else
		{
			throw hresult_not_implemented(L"A page that uses ExpandIntoTitlebar should override DragRegion.");
		}
	}

	wf::Rect FramelessPage::TitlebarButtonsRegion()
	{
		const bool closable = IsClosable();
		const auto titlebarButtons = TitlebarContent();
		if (!ExpandIntoTitlebar() || (closable == false && (!titlebarButtons || titlebarButtons.Size() == 0)))
		{
			return { 0.0f, 0.0f, 0.0f, 0.0f };
		}
		else
		{
			double height = 0.0f;
			double width = 0.0f;
			if (closable)
			{
				const auto closeButton = CloseButton();

				if (height == 0.0f)
				{
					height = closeButton.ActualHeight();
				}

				width += closeButton.ActualWidth();
			}

			if (titlebarButtons)
			{
				for (const auto button : titlebarButtons)
				{
					if (height == 0.0f)
					{
						height = button.ActualHeight();
					}

					width += button.ActualWidth();
				}
			}

			return {
				static_cast<float>(ActualWidth() - width),
				0.0f,
				static_cast<float>(width),
				static_cast<float>(height)
			};
		}
	}

	void FramelessPage::Close()
	{
		m_ClosedHandler();
	}

	void FramelessPage::CloseClicked(const IInspectable &, const wux::RoutedEventArgs &)
	{
		RequestClose();
	}
}
