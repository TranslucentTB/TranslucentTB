#pragma once
#include <algorithm>
#include <cstddef>
#include <cwctype>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>

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
	inline bool IgnoreCaseStringEquals(std::wstring_view l, std::wstring_view r)
	{
		return std::equal(l.begin(), l.end(), r.begin(), r.end(), [](wchar_t a, wchar_t b) -> bool
		{
			return std::towlower(a) == std::towlower(b);
		});
	}

	namespace impl {
		struct string_lowercase_hash {
			inline std::size_t operator()(std::wstring_view k) const noexcept
			{
				static const std::hash<std::wstring> hasher;
				return hasher(ToLower(std::wstring(k)));
			}
		};

		struct string_lowercase_compare {
			inline bool operator()(std::wstring_view l, std::wstring_view r) const noexcept
			{
				return IgnoreCaseStringEquals(l, r);
			}
		};

		// https://en.cppreference.com/w/cpp/string/wide/iswspace
		static constexpr std::wstring_view WHITESPACES = L" \f\n\r\t\v";
	}

	// Case-insensitive std::unordered_map with string keys.
	template<typename T>
	using string_view_map = std::unordered_map<const std::wstring_view, T, impl::string_lowercase_hash, impl::string_lowercase_compare>;

	// Removes instances of a character at the beginning and end of the string.
	constexpr std::wstring_view Trim(std::wstring_view str, std::wstring_view characters = impl::WHITESPACES)
	{
		const size_t first = str.find_first_not_of(characters);

		if (first == std::wstring_view::npos)
		{
			return { };
		}

		const size_t last = str.find_last_not_of(characters);
		return str.substr(first, last - first + 1);
	}

	// Removes instances of a character at the beginning and end of the string.
	constexpr void TrimInplace(std::wstring_view &str, std::wstring_view characters = impl::WHITESPACES)
	{
		const size_t first = str.find_first_not_of(characters);

		if (first == std::wstring_view::npos)
		{
			str = { };
		}

		str.remove_prefix(first);

		const size_t last = str.find_last_not_of(characters);
		str.remove_suffix(str.length() - last - 1);
	}

	// Removes instances of a character at the beginning and end of the string.
	inline void TrimInplace(std::wstring &str, std::wstring_view characters = impl::WHITESPACES)
	{
		const size_t first = str.find_first_not_of(characters);

		if (first == std::wstring::npos)
		{
			str.erase();
			return;
		}

		const size_t last = str.find_last_not_of(characters);
		str.erase(last + 1);
		str.erase(0, first);
	}

	// Checks if a string begins with another string. More efficient than str.find(text) == 0.
	constexpr bool StringBeginsWith(std::wstring_view string, std::wstring_view string_to_test)
	{
		const size_t length = string_to_test.length();
		if (string.length() < length)
		{
			return false;
		}

		for (size_t i = 0; i < length; i++)
		{
			if (string_to_test[i] != string[i])
			{
				return false;
			}
		}
		return true;
	}

	// Checks if a string begins with any of the strings in the second parameter.
	// T must be iteratable using a range-for with a type convertible to std::wstring_view.
	// For example std::vector<std::wstring> works, as well as IVectorView<winrt::hstring>.
	template<class T = std::initializer_list<std::wstring_view>>
	constexpr bool StringBeginsWithOneOf(std::wstring_view string, const T &strings_to_test)
	{
		for (const auto &string_to_test : strings_to_test)
		{
			if (StringBeginsWith(string, string_to_test))
			{
				return true;
			}
			else
			{
				continue;
			}
		}

		return false;
	}

	// Removes a string at the beginning of another string.
	constexpr std::wstring_view RemovePrefix(std::wstring_view str, std::wstring_view prefix)
	{
		if (StringBeginsWith(str, prefix))
		{
			str.remove_prefix(prefix.length());
		}

		return str;
	}

	// Removes a string at the beginning of another string.
	constexpr void RemovePrefixInplace(std::wstring_view &str, std::wstring_view prefix)
	{
		if (StringBeginsWith(str, prefix))
		{
			str.remove_prefix(prefix.length());
		}
	}

	// Removes a string at the beginning of another string.
	inline void RemovePrefixInplace(std::wstring &str, std::wstring_view prefix)
	{
		if (StringBeginsWith(str, prefix))
		{
			str.erase(0, prefix.length());
		}
	}
}