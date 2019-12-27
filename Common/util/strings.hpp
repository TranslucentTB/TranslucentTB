#pragma once

#define UTIL_WIDEN_INNER(x) L##x
#define UTIL_WIDEN(x) UTIL_WIDEN_INNER(x)

#define UTIL_STRINGIFY_INNER(x) #x
#define UTIL_STRINGIFY(x) UTIL_WIDEN(UTIL_STRINGIFY_INNER(x))
#define UTIL_STRINGIFY_UTF8(x) UTIL_STRINGIFY_INNER(x)

#ifndef RC_INVOKED

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_set>

#include "hash.hpp"

namespace Util {
	// Converts a string to its lowercase variant
	inline void ToLowerInplace(std::wstring &data)
	{
		std::transform(data.begin(), data.end(), data.begin(), std::towlower);
	}

	// Converts a string to its lowercase variant
	inline std::wstring ToLower(std::wstring data)
	{
		ToLowerInplace(data);
		return data;
	}

	// Checks if two strings are the same, ignoring case
	inline bool IgnoreCaseStringEquals(std::wstring_view l, std::wstring_view r) noexcept
	{
		return std::equal(l.begin(), l.end(), r.begin(), r.end(), [](wchar_t a, wchar_t b) noexcept
		{
			return std::towlower(a) == std::towlower(b);
		});
	}

	namespace impl {
		struct string_lowercase_compare {
			using is_transparent = void;
			inline bool operator()(std::wstring_view l, std::wstring_view r) const noexcept
			{
				return IgnoreCaseStringEquals(l, r);
			}
		};

		struct string_lowercase_hash {
			using transparent_key_equal = string_lowercase_compare;
			inline std::size_t operator()(std::wstring_view k) const noexcept
			{
				std::size_t initial = 0;
				for (const wchar_t character : k)
				{
					HashCombine(initial, std::towlower(character));
				}

				return initial;
			}
		};

		// https://en.cppreference.com/w/cpp/string/wide/iswspace
		static constexpr std::wstring_view WHITESPACES = L" \f\n\r\t\v";
	}

	// Case-insensitive string set.
	using string_set = std::unordered_set<std::wstring, impl::string_lowercase_hash>;

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

	// Checks if a string begins with any of the strings in the second parameter.
	template<class T = std::initializer_list<std::wstring_view>>
	constexpr bool StringBeginsWithOneOf(std::wstring_view string, const T &strings_to_test) noexcept
	{
		return std::any_of(std::begin(strings_to_test), std::end(strings_to_test), [string](const auto &a) noexcept -> bool
		{
			return string.starts_with(a);
		});
	}

	// Removes a string at the beginning of another string.
	constexpr std::wstring_view RemovePrefix(std::wstring_view str, std::wstring_view prefix)
	{
		if (str.starts_with(prefix))
		{
			str.remove_prefix(prefix.length());
		}

		return str;
	}

	// Removes a string at the beginning of another string.
	constexpr void RemovePrefixInplace(std::wstring_view &str, std::wstring_view prefix)
	{
		if (str.starts_with(prefix))
		{
			str.remove_prefix(prefix.length());
		}
	}

	// Removes a string at the beginning of another string.
	inline void RemovePrefixInplace(std::wstring &str, std::wstring_view prefix)
	{
		if (str.starts_with(prefix))
		{
			str.erase(0, prefix.length());
		}
	}
}

#endif // !RC_INVOKED
