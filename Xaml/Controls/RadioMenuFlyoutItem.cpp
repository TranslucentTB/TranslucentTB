#include "pch.h"
#include <wil/resource.h>

#include "RadioMenuFlyoutItem.h"
#if __has_include("Controls/RadioMenuFlyoutItem.g.cpp")
#include "Controls/RadioMenuFlyoutItem.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	RadioMenuFlyoutItem::RadioMenuFlyoutItem()
	{
		m_PropertyChangedToken.value = RegisterPropertyChangedCallback(wuxc::ToggleMenuFlyoutItem::IsCheckedProperty(), { this, &RadioMenuFlyoutItem::IsCheckedChanged });
	}

	void RadioMenuFlyoutItem::ProgrammaticUncheck()
	{
		m_IsSafeUncheck = true;
		const auto guard = wil::scope_exit([this]
		{
			m_IsSafeUncheck = false;
		});

		IsChecked(false);
	}

	RadioMenuFlyoutItem::~RadioMenuFlyoutItem()
	{
		UnregisterPropertyChangedCallback(wuxc::ToggleMenuFlyoutItem::IsCheckedProperty(), m_PropertyChangedToken.value);
	}

	void RadioMenuFlyoutItem::IsCheckedChanged(const wux::DependencyObject &, const wux::DependencyProperty &)
	{
		if (IsChecked())
		{
			UpdateOtherRadios();
		}
		else if (!m_IsSafeUncheck)
		{
			// don't allow the user to uncheck
			IsChecked(true);
		}
	}

	void RadioMenuFlyoutItem::UpdateOtherRadios()
	{
		// Since this item is checked, uncheck all siblings
		if (const auto parent = wux::Media::VisualTreeHelper::GetParent(*this))
		{
			const int childrenCount = wux::Media::VisualTreeHelper::GetChildrenCount(parent);
			for (int i = 0; i < childrenCount; i++)
			{
				const auto radioItem = wux::Media::VisualTreeHelper::GetChild(parent, i).try_as<RadioMenuFlyoutItem>();
				if (radioItem && radioItem.get() != this && radioItem->GroupName() == GroupName())
				{
					radioItem->ProgrammaticUncheck();
				}
			}
		}
	}
}
