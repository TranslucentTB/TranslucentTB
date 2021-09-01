#include "pch.h"
#include <tuple>

#include "FluentColorPalette.h"
#if __has_include("Models/FluentColorPalette.g.cpp")
#include "Models/FluentColorPalette.g.cpp"
#endif

namespace winrt::TranslucentTB::Xaml::Models::implementation
{
	uint32_t FluentColorPalette::ColorCount() noexcept
	{
		return std::tuple_size_v<decltype(COLOR_CHART)>;
	}

	uint32_t FluentColorPalette::ShadeCount() noexcept
	{
		return std::tuple_size_v<decltype(COLOR_CHART)::value_type>;
	}

	Windows::UI::Color FluentColorPalette::GetColor(uint32_t colorIndex, uint32_t shadeIndex)
	{
		return COLOR_CHART.at(colorIndex).at(shadeIndex);
	}
}
