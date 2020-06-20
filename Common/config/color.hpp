#pragma once
#include <cstdint>
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <winrt/Windows.UI.h>

#include "../util/numbers.hpp"
#include "../util/strings.hpp"

struct Color {
	uint8_t R, G, B, A;

	constexpr Color() noexcept : R(0), G(0), B(0), A(0) { }
	constexpr Color(uint8_t r, uint8_t g, uint8_t b) noexcept : R(r), G(g), B(b), A(255) { }
	constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept : R(r), G(g), B(b), A(a) { }
	constexpr Color(winrt::Windows::UI::Color col) noexcept : R(col.R), G(col.G), B(col.B), A(col.A) { }

#pragma warning(push)
#pragma warning(disable: 4244)
	constexpr explicit Color(uint32_t col) noexcept : R((col & 0xFF000000) >> 24), G((col & 0xFF0000) >> 16), B((col & 0xFF00) >> 8), A(col & 0xFF) { }
#pragma warning(pop)

	constexpr uint32_t ToRGBA() const noexcept
	{
		return ToPacked(R, G, B, A);
	}

	constexpr uint32_t ToABGR() const noexcept
	{
		return ToPacked(A, B, G, R);
	}

	template<std::size_t size>
	inline void ToString(fmt::basic_memory_buffer<wchar_t, size> &buf) const
	{
		fmt::format_to(buf, fmt(L"#{:08X}"), ToRGBA());
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
			const uint16_t col = Util::ParseHexNumber<uint16_t>(str) & 0xFFFF;
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
		return R == other.R && G == other.G && B == other.B && A == other.A;
	}

private:
	static constexpr uint32_t ToPacked(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth) noexcept
	{
		return (uint32_t(first) << 24) | (uint32_t(second) << 16) | (uint32_t(third) << 8) | uint32_t(fourth);
	}
};
