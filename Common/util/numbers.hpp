#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>

#include "strings.hpp"

namespace Util {
	namespace impl {
		constexpr bool IsDecimalDigit(wchar_t character)
		{
			return character >= L'0' && character <= L'9';
		}

		constexpr bool IsUpperHexDigit(wchar_t character)
		{
			return character >= L'A' && character <= L'F';
		}

		constexpr bool IsLowerHexDigit(wchar_t character)
		{
			return character >= L'a' && character <= L'f';
		}
	}

	// Apparently no wide string to number parser accepted an explicit ending to the string
	// so here I am. Also C locales sucks.
	template<std::unsigned_integral T = uint32_t>
	constexpr T ParseHexNumber(std::wstring_view number)
	{
		TrimInplace(number);

		if (number.empty())
		{
			throw std::invalid_argument("Cannot convert empty string to number");
		}

		if (number.length() > 2 && number[0] == L'0' && (number[1] == L'x' || number[1] == L'X'))
		{
			number.remove_prefix(2);
		}

		if (number.length() > std::numeric_limits<T>::digits / 4)
		{
			throw std::out_of_range("Number being converted is off-limits");
		}

		T result { };
		for (std::size_t i = 0; i < number.length(); i++)
		{
			const T power = static_cast<T>(1) << ((number.length() - i - 1) * 4);
			if (impl::IsDecimalDigit(number[i]))
			{
				result += static_cast<T>(number[i] - L'0') * power;
			}
			else if (impl::IsUpperHexDigit(number[i]))
			{
				result += static_cast<T>(number[i] - L'A' + 10) * power;
			}
			else if (impl::IsLowerHexDigit(number[i]))
			{
				result += static_cast<T>(number[i] - L'a' + 10) * power;
			}
			else
			{
				throw std::invalid_argument("Not a number");
			}
		}

		return result;
	}

	constexpr uint8_t ExpandOneHexDigitByte(uint8_t byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}
}
