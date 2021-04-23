#include "pch.h"

#include "FlyoutPage.h"
#if __has_include("Pages/FlyoutPage.g.cpp")
#include "Pages/FlyoutPage.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Pages::implementation
{
	void FlyoutPage::SetTooltipVisible(bool visible)
	{
		if (const auto tooltip = wuxc::ToolTipService::GetToolTip(*this).try_as<wuxc::ToolTip>())
		{
			tooltip.IsOpen(visible);
		}
	}
}
