#pragma once
#include <concepts>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "strings.hpp"

namespace Util {
	namespace impl {
		constexpr bool IsDecimalDigit(wchar_t character)
		{
			return character >= L'0' && character <= L'9';
		}

		constexpr bool IsCapitalHexDigit(wchar_t character)
		{
			return character >= L'A' && character <= L'F';
		}

		constexpr bool IsLowerHexDigit(wchar_t character)
		{
			return character >= L'a' && character <= L'f';
		}

		template<std::Integral T>
		constexpr T pow(uint8_t base, std::size_t exponent)
		{
			if (exponent == 0)
			{
				return 1;
			}
			else
			{
				T result = base;
				for (std::size_t i = exponent - 1; i > 0; i--)
				{
					result *= base;
				}
				return result;
			}
		}

		template<std::Integral T, uint8_t base>
		struct NumberParser;

		template<std::Integral T>
		struct NumberParser<T, 10> {
			static constexpr T impl(std::wstring_view number)
			{
				bool isNegative = false;
				if (number.length() > 1 && number[0] == L'-')
				{
					if constexpr (std::is_signed_v<T>)
					{
						number.remove_prefix(1);
						isNegative = true;
					}
					else
					{
						throw std::out_of_range("Cannot convert a negative number to a unsigned integer");
					}
				}

				T result {};
				for (std::size_t i = 0; i < number.length(); i++)
				{
					if (impl::IsDecimalDigit(number[i]))
					{
						const T power = impl::pow<T>(10, number.length() - i - 1);
						result += (number[i] - L'0') * power;
					}
					else
					{
						throw std::invalid_argument("Not a number");
					}
				}

				if constexpr (std::is_signed_v<T>)
				{
					return isNegative ? -result : result;
				}
				else
				{
					return result;
				}
			}
		};

		template<std::UnsignedIntegral T>
		struct NumberParser<T, 16> {
			static constexpr T impl(std::wstring_view number)
			{
				if (number.length() > 2 && number[0] == L'0' && (number[1] == L'x' || number[1] == L'X'))
				{
					number.remove_prefix(2);
				}

				T result {};
				for (std::size_t i = 0; i < number.length(); i++)
				{
					const T power = 1 << ((number.length() - i - 1) * 4);
					if (impl::IsDecimalDigit(number[i]))
					{
						result += (number[i] - L'0') * power;
					}
					else if (impl::IsCapitalHexDigit(number[i]))
					{
						result += (number[i] - L'A' + 10) * power;
					}
					else if (impl::IsLowerHexDigit(number[i]))
					{
						result += (number[i] - L'a' + 10) * power;
					}
					else
					{
						throw std::invalid_argument("Not a number");
					}
				}

				return result;
			}
		};
	}

	// Apparently no wide string to number parser accepted an explicit ending to the string
	// so here I am. Also C locales sucks.
	template<std::Integral T = int32_t, uint8_t base = 10>
	constexpr T ParseNumber(std::wstring_view number)
	{
		return impl::NumberParser<T, base>::impl(number);
	}

	constexpr uint8_t ExpandOneHexDigitByte(uint8_t byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}
}
