#include "pch.h"

#include "FunctionalConverters.h"
#if __has_include("FunctionalConverters.g.cpp")
#include "FunctionalConverters.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::implementation
{
	wux::Visibility FunctionalConverters::InvertedBoolToVisibility(bool value) noexcept
	{
		return value ? wux::Visibility::Collapsed : wux::Visibility::Visible;
	}

	bool FunctionalConverters::IsSameLogSinkState(txmp::LogSinkState a, txmp::LogSinkState b) noexcept
	{
		return a == b;
	}

	bool FunctionalConverters::IsDifferentLogSinkState(txmp::LogSinkState a, txmp::LogSinkState b) noexcept
	{
		return a != b;
	}
}
