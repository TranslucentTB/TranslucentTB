#include "pch.h"
#include <algorithm>

#include "FramelessPage.h"
#if __has_include("Pages/FramelessPage.g.cpp")
#include "Pages/FramelessPage.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	wux::DependencyProperty FramelessPage::s_SystemMenuContentProperty =
		wux::DependencyProperty::Register(
			L"SystemMenuContent",
			winrt::xaml_typename<wfc::IObservableVector<wuxc::MenuFlyoutItemBase>>(),
			winrt::xaml_typename<class_type>(),
			wux::PropertyMetadata(nullptr, SystemMenuContentChanged));

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

	void FramelessPage::SystemMenuOpening(const IInspectable &, const IInspectable &)
	{
		if (m_NeedsSystemMenuRefresh)
		{
			const auto menuItems = SystemMenu().Items();

			// remove everything but the Close menu item
			const auto closeItem = CloseMenuItem();
			menuItems.Clear();
			menuItems.Append(closeItem);

			bool needsMergeStyle = false;
			const auto content = SystemMenuContent();
			if (content && content.Size() > 0)
			{
				menuItems.InsertAt(0, wuxc::MenuFlyoutSeparator());
				std::ranges::for_each(
					std::make_reverse_iterator(end(content)),
					std::make_reverse_iterator(begin(content)),
					[&menuItems, &needsMergeStyle](const auto &newItem)
					{
						if (newItem.try_as<wuxc::ToggleMenuFlyoutItem>())
						{
							needsMergeStyle = true;
						}

						menuItems.InsertAt(0, newItem);
					});
			}

			closeItem.Style(needsMergeStyle ? LookupStyle(winrt::box_value(L"MergeIconsMenuFlyoutItem")) : nullptr);

			m_NeedsSystemMenuRefresh = false;
		}
	}

	void FramelessPage::SystemMenuChanged(const wfc::IObservableVector<wuxc::MenuFlyoutItemBase> &, const wfc::IVectorChangedEventArgs &)
	{
		m_NeedsSystemMenuRefresh = true;
	}

	wux::Style FramelessPage::LookupStyle(const IInspectable &key)
	{
		return wux::Application::Current().Resources().TryLookup(key).try_as<wux::Style>();
	}

	void FramelessPage::SystemMenuContentChanged(const Windows::UI::Xaml::DependencyObject &d, const Windows::UI::Xaml::DependencyPropertyChangedEventArgs &e)
	{
		if (const auto page = d.try_as<FramelessPage>())
		{
			page->m_NeedsSystemMenuRefresh = true;

			page->m_SystemMenuChangedRevoker.revoke();
			if (const auto vector = e.NewValue().try_as<wfc::IObservableVector<wuxc::MenuFlyoutItemBase>>())
			{
				page->m_SystemMenuChangedRevoker = vector.VectorChanged(winrt::auto_revoke, { page->get_weak(), &FramelessPage::SystemMenuChanged });
			}
		}
	}
}
