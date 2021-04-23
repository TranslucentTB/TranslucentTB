#pragma once
#include "../factory.h"

#include "Pages/FlyoutPage.g.h"

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	struct FlyoutPage : FlyoutPageT<FlyoutPage>
	{
		void SetTooltipVisible(bool visible);
	};
}

FACTORY(winrt::TranslucentTB::Xaml::Pages, FlyoutPage);
