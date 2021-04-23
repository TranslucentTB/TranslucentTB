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
		static bool IsSameLogSinkState(Models::Primitives::LogSinkState a, Models::Primitives::LogSinkState b) noexcept;
		static bool IsDifferentLogSinkState(Models::Primitives::LogSinkState a, Models::Primitives::LogSinkState b) noexcept;
	};
}

FACTORY(winrt::TranslucentTB::Xaml, FunctionalConverters);
