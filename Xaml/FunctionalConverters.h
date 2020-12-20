#pragma once
#include "factory.h"
#include "winrt.hpp"

#include "FunctionalConverters.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct FunctionalConverters
	{
		static wux::Visibility InvertedBoolToVisibility(bool value);
	};
}

FACTORY(winrt::TranslucentTB::Xaml, FunctionalConverters);
