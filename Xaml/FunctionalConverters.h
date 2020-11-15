#pragma once
#include "factory.h"

#include "FunctionalConverters.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct FunctionalConverters
	{
		static Windows::UI::Xaml::Visibility InvertedBoolToVisibility(bool value);
	};
}

FACTORY(winrt::TranslucentTB::Xaml, FunctionalConverters);
