#pragma once

#include "FunctionalConverters.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct FunctionalConverters
	{
		static Windows::UI::Xaml::Visibility InvertedBoolToVisibility(bool value);
	};
}

namespace winrt::TranslucentTB::Xaml::factory_implementation
{
	struct FunctionalConverters : FunctionalConvertersT<FunctionalConverters, implementation::FunctionalConverters>
	{
	};
}
