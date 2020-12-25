#include "pch.h"

#include "FunctionalConverters.h"
#if __has_include("FunctionalConverters.g.cpp")
#include "FunctionalConverters.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::implementation
{
	wux::Visibility FunctionalConverters::InvertedBoolToVisibility(bool value)
	{
		return value ? wux::Visibility::Collapsed : wux::Visibility::Visible;
	}
}
