#pragma once
#include <cstdint>
#include <fmt/format.h>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <string_view>

#include "numbers.hpp"
#include "strings.hpp"

namespace Util {
	inline uint32_t ColorFromString(std::wstring_view str)
	{
		Util::TrimInplace(str);

		if (str.starts_with(L'#'))
		{
			str.remove_prefix(1);
		}
		else if (str.starts_with(L"0x"))
		{
			str.remove_prefix(2);
		}
		else
		{
			throw std::invalid_argument("not a valid color");
		}

		if (str.length() == 3)
		{
			const auto col = Util::ParseNumber<uint16_t, 16>(str) & 0xFFF;
			const uint32_t r = Util::ExpandOneHexDigitByte((col & 0xF00) >> 8);
			const uint32_t g = Util::ExpandOneHexDigitByte((col & 0xF0) >> 4);
			const uint32_t b = Util::ExpandOneHexDigitByte(col & 0xF);

			return (r << 16) + (g << 8) + b;
		}
		else if (str.length() == 6)
		{
			return Util::ParseNumber<uint32_t, 16>(str) & 0xFFFFFF;
		}
		else
		{
			throw std::invalid_argument("not a valid color");
		}
	}

	inline std::wstring StringFromColor(uint32_t color)
	{
		return fmt::format(L"#{:06x}", color & 0xFFFFFF);
	}

	constexpr uint32_t SwapColorEndian(uint32_t color)
	{
		const uint8_t left = (color & 0xFF0000) >> 16;
		const uint8_t right = color & 0xFF;

		return (color & 0xFF00) + (right << 16) + left;
	}
}