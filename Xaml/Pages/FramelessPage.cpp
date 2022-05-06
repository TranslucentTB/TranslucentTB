#include "pch.h"
#include <ranges>

#include "FramelessPage.h"
#if __has_include("Pages/FramelessPage.g.cpp")
#include "Pages/FramelessPage.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	FramelessPage::FramelessPage()
	{
		m_SystemMenuChangedToken = m_SystemMenuContent.VectorChanged({ get_weak(), &FramelessPage::SystemMenuChanged });

		// keeping InitializeComponent in the constructor as a workaround for https://github.com/microsoft/cppwinrt/issues/1140
		InitializeComponent();
	}

	bool FramelessPage::CanMove() noexcept
	{
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
		if (!ExpandIntoTitlebar() || (closable == false && m_TitlebarContent.Size() == 0))
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

			for (const auto button : m_TitlebarContent)
			{
				if (height == 0.0f)
				{
					height = button.ActualHeight();
				}

				width += button.ActualWidth();
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
			if (m_SystemMenuContent.Size() > 0)
			{
				menuItems.InsertAt(0, wuxc::MenuFlyoutSeparator());
				for (const auto newItem : m_SystemMenuContent | std::views::reverse)
				{
					if (newItem.try_as<wuxc::ToggleMenuFlyoutItem>())
					{
						needsMergeStyle = true;
					}

					menuItems.InsertAt(0, newItem);
				}
			}

			closeItem.Style(needsMergeStyle ? LookupStyle(winrt::box_value(L"MergeIconsMenuFlyoutItem")) : nullptr);

			m_NeedsSystemMenuRefresh = false;
		}
	}

	FramelessPage::~FramelessPage()
	{
		m_SystemMenuContent.VectorChanged(m_SystemMenuChangedToken);
	}

	void FramelessPage::SystemMenuChanged(const wfc::IObservableVector<wuxc::MenuFlyoutItemBase> &, const wfc::IVectorChangedEventArgs &)
	{
		m_NeedsSystemMenuRefresh = true;
	}

	wux::Style FramelessPage::LookupStyle(const IInspectable &key)
	{
		return wux::Application::Current().Resources().TryLookup(key).try_as<wux::Style>();
	}
}
