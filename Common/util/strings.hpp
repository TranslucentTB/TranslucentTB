#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>

namespace Util {
	namespace impl {
		// https://en.cppreference.com/w/cpp/string/wide/iswspace
		static constexpr std::wstring_view WHITESPACES = L" \f\n\r\t\v";
	}

	constexpr bool IsAscii(wchar_t c)
	{
		return (c & ~0x007F) == 0;
	}

	constexpr wchar_t AsciiToUpper(wchar_t c)
	{
		assert(IsAscii(c));

		const wchar_t lower = c + 0x0080 - 0x0061;
		const wchar_t upper = c + 0x0080 - 0x007B;
		const wchar_t combined = lower ^ upper;
		const wchar_t mask = (combined & 0x0080) >> 2;

		return c ^ mask;
	}

	// Removes instances of a character at the beginning and end of the string.
	constexpr std::wstring_view Trim(std::wstring_view str, std::wstring_view characters = impl::WHITESPACES)
	{
		const std::size_t first = str.find_first_not_of(characters);

		if (first != std::wstring_view::npos)
		{
			const std::size_t last = str.find_last_not_of(characters);
			return str.substr(first, last - first + 1);
		}
		else
		{
			return { };
		}
	}

	// Removes instances of a character at the beginning and end of the string.
	constexpr void TrimInplace(std::wstring_view &str, std::wstring_view characters = impl::WHITESPACES)
	{
		const std::size_t first = str.find_first_not_of(characters);

		if (first != std::wstring_view::npos)
		{
			str.remove_prefix(first);

			const std::size_t last = str.find_last_not_of(characters);
			str.remove_suffix(str.length() - last - 1);
		}
		else
		{
			str = { };
		}
	}

	// Removes instances of a character at the beginning and end of the string.
	inline void TrimInplace(std::wstring &str, std::wstring_view characters = impl::WHITESPACES)
	{
		const std::size_t first = str.find_first_not_of(characters);

		if (first != std::wstring::npos)
		{
			const std::size_t last = str.find_last_not_of(characters);
			str.erase(last + 1);
			str.erase(0, first);
		}
		else
		{
			str.erase();
		}
	}
}
