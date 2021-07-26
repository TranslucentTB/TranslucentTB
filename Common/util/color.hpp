#pragma once
#include <bit>
#include <cstdint>
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include "../winrt.hpp"
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.UI.h>

#include "numbers.hpp"
#include "strings.hpp"

#if __has_include(<winrt/TranslucentTB.Xaml.Models.Primitives.h>)
#define HAS_WINRT_COLOR
#include <winrt/TranslucentTB.Xaml.Models.Primitives.h>
#endif

namespace Util {
	struct HsvColor {
		double H, S, V, A;

		constexpr HsvColor() noexcept : H(0), S(0), V(0), A(0) { }
		constexpr HsvColor(double h, double s, double v, double a = 1.0) noexcept : H(h), S(s), V(v), A(a) { }

#ifdef HAS_WINRT_COLOR
		constexpr HsvColor(txmp::HsvColor col) noexcept : H(col.H), S(col.S), V(col.V), A(col.A) { }

		constexpr operator txmp::HsvColor() const noexcept
		{
			return { .H = H, .S = S, .V = V, .A = A };
		}
#endif

		constexpr operator wf::Numerics::float4() const noexcept
		{
			return {
				static_cast<float>(H),
				static_cast<float>(S),
				static_cast<float>(V),
				static_cast<float>(A)
			};
		}
	};

	struct Color {
		uint8_t R, G, B, A;

		constexpr Color() noexcept : R(0), G(0), B(0), A(0) { }
		constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept : R(r), G(g), B(b), A(a) { }
		constexpr Color(winrt::Windows::UI::Color col) noexcept : Color(std::bit_cast<Color>(std::rotr(std::bit_cast<uint32_t>(col), 8))) { }
		constexpr explicit Color(uint32_t col) noexcept : Color(std::bit_cast<Color>(SwapBytes(col))) { }

		constexpr uint32_t ToRGBA() const noexcept
		{
			return SwapBytes(ToABGR());
		}

		constexpr uint32_t ToABGR() const noexcept
		{
			return std::bit_cast<uint32_t>(*this);
		}

		constexpr Color Premultiply() const noexcept
		{
			return {
				FastPremultiply(R, A),
				FastPremultiply(G, A),
				FastPremultiply(B, A),
				A
			};
		}

		inline HsvColor ToHSV() const
		{
			static constexpr double toDouble = 1.0 / 255.0;

			const double r = toDouble * R;
			const double g = toDouble * G;
			const double b = toDouble * B;
			const double max = std::max({ r, g, b });
			const double min = std::min({ r, g, b });
			const double chroma = max - min;
			double h1;

			if (chroma == 0.0)
			{
				h1 = 0.0;
			}
			else if (max == r)
			{
				// fmod doesn't do proper modulo on negative
				// numbers, so we'll add 6 before using it
				h1 = std::fmod(((g - b) / chroma) + 6.0, 6.0);
			}
			else if (max == g)
			{
				h1 = 2.0 + ((b - r) / chroma);
			}
			else
			{
				h1 = 4.0 + ((r - g) / chroma);
			}

			const double saturation = chroma == 0.0 ? 0.0 : chroma / max;

			return { 60.0 * h1, saturation, max, toDouble * A };
		}

		template<std::size_t size>
		inline void ToString(fmt::basic_memory_buffer<wchar_t, size>& buf) const
		{
			fmt::format_to(buf, FMT_STRING(L"#{:08X}"), ToRGBA());
		}

		constexpr static Color FromString(std::wstring_view str, bool allowNoPrefix = false)
		{
			Util::TrimInplace(str);

			if (str.starts_with(L'#'))
			{
				str.remove_prefix(1);
			}
			else if (!allowNoPrefix)
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

		inline static Color FromHSV(double hue, double saturation, double value, double alpha = 1.0)
		{
			if (hue < 0 || hue > 360)
			{
				throw std::out_of_range("hue must be between 0 and 360");
			}

			const double chroma = value * saturation;
			const double h1 = hue / 60.0;
			const double x = chroma * (1.0 - std::abs(std::fmod(h1, 2.0) - 1.0));
			const double m = value - chroma;
			double r1, g1, b1;

			if (h1 < 1.0)
			{
				r1 = chroma;
				g1 = x;
				b1 = 0.0;
			}
			else if (h1 < 2.0)
			{
				r1 = x;
				g1 = chroma;
				b1 = 0.0;
			}
			else if (h1 < 3.0)
			{
				r1 = 0.0;
				g1 = chroma;
				b1 = x;
			}
			else if (h1 < 4.0)
			{
				r1 = 0.0;
				g1 = x;
				b1 = chroma;
			}
			else if (h1 < 5.0)
			{
				r1 = x;
				g1 = 0.0;
				b1 = chroma;
			}
			else
			{
				r1 = chroma;
				g1 = 0.0;
				b1 = x;
			}

			return {
				static_cast<uint8_t>(255.0 * (r1 + m)),
				static_cast<uint8_t>(255.0 * (g1 + m)),
				static_cast<uint8_t>(255.0 * (b1 + m)),
				static_cast<uint8_t>(255.0 * alpha)
			};
		}

		inline static Color FromHSV(const HsvColor &hsvColor)
		{
			return FromHSV(hsvColor.H, hsvColor.S, hsvColor.V, hsvColor.A);
		}

		constexpr operator winrt::Windows::UI::Color() const noexcept
		{
			return std::bit_cast<winrt::Windows::UI::Color>(std::rotl(ToABGR(), 8));
		}

		constexpr bool operator ==(Color other) const noexcept
		{
			return ToABGR() == other.ToABGR();
		}

	private:
		static constexpr uint8_t FastPremultiply(uint8_t x, uint8_t frac) noexcept
		{
			// magic that avoids a division.
			return static_cast<uint8_t>(static_cast<uint32_t>(x * frac * 0x8081) >> 23);
		}

		static constexpr uint32_t SwapBytes(uint32_t num) noexcept
		{
			return (num >> 24) | ((num & 0xFF0000) >> 8) | ((num & 0xFF00) << 8) | (num << 24);
		}
	};
}
