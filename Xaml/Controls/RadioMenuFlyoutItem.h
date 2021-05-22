#pragma once
#include "../dependencyproperty.h"
#include "../factory.h"

#include "Controls/RadioMenuFlyoutItem.g.h"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	struct RadioMenuFlyoutItem : RadioMenuFlyoutItemT<RadioMenuFlyoutItem>
	{
		RadioMenuFlyoutItem();

		DECL_DEPENDENCY_PROPERTY_WITH_DEFAULT(hstring, GroupName, box_value(L""));

		void ProgrammaticUncheck();

		~RadioMenuFlyoutItem();

	private:
		void IsCheckedChanged(const wux::DependencyObject &, const wux::DependencyProperty &);
		void UpdateOtherRadios();

		event_token m_PropertyChangedToken;
		bool m_IsSafeUncheck = false;
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Controls, RadioMenuFlyoutItem);
