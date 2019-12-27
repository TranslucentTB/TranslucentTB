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

		constexpr bool IsUpperHexDigit(wchar_t character)
		{
			return character >= L'A' && character <= L'F';
		}

		constexpr bool IsLowerHexDigit(wchar_t character)
		{
			return character >= L'a' && character <= L'f';
		}

		template<std::signed_integral T>
		constexpr T abs(T num)
		{
			if (num == std::numeric_limits<T>::min())
			{
				throw std::out_of_range("Cannot get absolute value of minimum number");
			}

			return num >= 0 ? num : -num;
		}

		template<std::unsigned_integral T>
		constexpr T abs(T num)
		{
			return num;
		}

		template<std::integral T, uint8_t base>
		struct NumberParser;

		template<std::integral T>
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

				constexpr T max = std::numeric_limits<T>::max();
				constexpr T maxmthresh = max / 10;
				constexpr wchar_t maxdthresh = (max % 10) + L'0';

				constexpr T min = std::numeric_limits<T>::min();
				constexpr T minmthresh = min / 10;
				constexpr wchar_t mindthresh = static_cast<wchar_t>(abs(min % 10)) + L'0';

				T result {};
				for (const wchar_t digit : number)
				{
					if (result > maxmthresh || (result == maxmthresh && digit > maxdthresh) || (isNegative && (result < minmthresh || (result == minmthresh && digit > mindthresh))))
					{
						throw std::out_of_range("Number being converted is off-limits");
					}
					else
					{
						if (impl::IsDecimalDigit(digit))
						{
							result *= 10;
							const T intDigit = static_cast<T>(digit - L'0');
							if (isNegative)
							{
								result -= intDigit;
							}
							else
							{
								result += intDigit;
							}
						}
						else
						{
							throw std::invalid_argument("Not a number");
						}
					}
				}

				return result;
			}
		};

		template<std::unsigned_integral T>
		struct NumberParser<T, 16> {
			static constexpr T impl(std::wstring_view number)
			{
				if (number.length() > 2 && number[0] == L'0' && (number[1] == L'x' || number[1] == L'X'))
				{
					number.remove_prefix(2);
				}

				if (number.length() > std::numeric_limits<T>::digits / 4)
				{
					throw std::out_of_range("Number being converted is off-limits");
				}

				T result {};
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
		};
	}

	// Apparently no wide string to number parser accepted an explicit ending to the string
	// so here I am. Also C locales sucks.
	template<std::integral T = int32_t, uint8_t base = 10>
	constexpr T ParseNumber(std::wstring_view number)
	{
		return impl::NumberParser<T, base>::impl(Trim(number));
	}

	constexpr uint8_t ExpandOneHexDigitByte(uint8_t byte)
	{
		const uint8_t firstDigit = byte & 0xF;
		return (firstDigit << 4) + firstDigit;
	}
}
