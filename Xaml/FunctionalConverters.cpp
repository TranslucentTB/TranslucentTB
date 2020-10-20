#include "pch.h"

#include "FunctionalConverters.h"
#if __has_include("FunctionalConverters.g.cpp")
#include "FunctionalConverters.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TranslucentTB::Xaml::implementation
{
	Visibility FunctionalConverters::InvertedBoolToVisibility(bool value)
	{
		return value ? Visibility::Collapsed : Visibility::Visible;
	}
}
