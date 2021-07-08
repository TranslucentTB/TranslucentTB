#pragma once
#include "factory.h"
#include "winrt.hpp"
#include "winrt/TranslucentTB.Xaml.Models.Primitives.h"

#include "FunctionalConverters.g.h"

namespace winrt::TranslucentTB::Xaml::implementation
{
	struct FunctionalConverters
	{
		static wux::Visibility InvertedBoolToVisibility(bool value) noexcept;
		static bool IsSameLogSinkState(txmp::LogSinkState a, txmp::LogSinkState b) noexcept;
		static bool IsDifferentLogSinkState(txmp::LogSinkState a, txmp::LogSinkState b) noexcept;
	};
}

FACTORY(winrt::TranslucentTB::Xaml, FunctionalConverters);
