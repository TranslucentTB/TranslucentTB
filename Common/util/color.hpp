#pragma once
#include <bit>
#include <cstdint>
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include "../winrt.hpp"
#include <winrt/Windows.UI.h>

#include "numbers.hpp"
#include "strings.hpp"

namespace Util {
	struct Color {
		uint8_t R, G, B, A;

		constexpr Color() noexcept : R(0), G(0), B(0), A(0) { }
		constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept : R(r), G(g), B(b), A(a) { }
		constexpr Color(winrt::Windows::UI::Color col) noexcept : R(col.R), G(col.G), B(col.B), A(col.A) { }
		constexpr explicit Color(uint32_t col) noexcept : Color(std::bit_cast<Color>(SwapBytes(col))) { }

		constexpr uint32_t ToRGBA() const noexcept
		{
			return SwapBytes(ToABGR());
		}

		constexpr uint32_t ToABGR() const noexcept
		{
			return std::bit_cast<uint32_t>(*this);
		}

		template<std::size_t size>
		inline void ToString(fmt::basic_memory_buffer<wchar_t, size>& buf) const
		{
			fmt::format_to(buf, FMT_STRING(L"#{:08X}"), ToRGBA());
		}

		inline static Color FromString(std::wstring_view str)
		{
			Util::TrimInplace(str);

			if (str.starts_with(L'#'))
			{
				str.remove_prefix(1);
			}
			else
			{
				throw std::invalid_argument("Not a valid color");
			}

			if (str.length() == 3)
			{
				const uint16_t col = Util::ParseHexNumber<uint16_t>(str) & 0xFFF;
				const uint8_t r = Util::ExpandOneHexDigitByte((col & 0xF00) >> 8);
				const uint8_t g = Util::ExpandOneHexDigitByte((col & 0xF0) >> 4);
				const uint8_t b = Util::ExpandOneHexDigitByte(col & 0xF);

				return Color { r, g, b };
			}
			else if (str.length() == 4)
			{
				const auto col = Util::ParseHexNumber<uint16_t>(str);
				const uint8_t r = Util::ExpandOneHexDigitByte((col & 0xF000) >> 12);
				const uint8_t g = Util::ExpandOneHexDigitByte((col & 0xF00) >> 8);
				const uint8_t b = Util::ExpandOneHexDigitByte((col & 0xF0) >> 4);
				const uint8_t a = Util::ExpandOneHexDigitByte(col & 0xF);

				return Color { r, g, b, a };
			}
			else if (str.length() == 6)
			{
				return Color { Util::ParseHexNumber<uint32_t>(str) << 8 | 0xFF };
			}
			else if (str.length() == 8)
			{
				return Color { Util::ParseHexNumber<uint32_t>(str) };
			}
			else
			{
				throw std::invalid_argument("Not a valid color");
			}
		}

		constexpr operator winrt::Windows::UI::Color() const noexcept
		{
			return { .A = A, .R = R, .G = G, .B = B };
		}

		constexpr bool operator ==(Color other) const noexcept
		{
			return ToABGR() == other.ToABGR();
		}

	private:
		static constexpr uint32_t SwapBytes(uint32_t num) noexcept
		{
			return (num >> 24) | ((num & 0xFF0000) >> 8) | ((num & 0xFF00) << 8) | (num << 24);
		}
	};
}
