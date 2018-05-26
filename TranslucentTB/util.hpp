#pragma once
#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <limits>
#include <random>
#include <string>
#include <unordered_map>

class Util {

public:
	// Converts a string to its lowercase variant
	inline static std::wstring ToLower(const std::wstring &data)
	{
		std::wstring lower;
		lower.resize(data.length());
		std::transform(data.begin(), data.end(), lower.begin(), std::towlower);
		return lower;
	}

	// Converts a string to its lowercase variant
	inline static void ToLowerInplace(std::wstring &data)
	{
		std::transform(data.begin(), data.end(), data.begin(), std::towlower);
	}

private:
	struct string_hash {
		std::hash<std::wstring> m_Hasher;
		inline std::size_t operator()(const std::wstring &k) const
		{
			return m_Hasher(ToLower(k));
		}
	};

	struct string_compare {
		inline bool operator()(const std::wstring &l, const std::wstring &r) const
		{
			return l.size() == r.size() &&
				std::equal(l.begin(), l.end(), r.begin(), [](const wchar_t &a, const wchar_t &b) -> bool
				{
					return std::towlower(a) == std::towlower(b);
				});
		}
	};

public:
	// Case-insensitive std::unordered_map with string keys.
	template<typename T>
	using string_map = std::unordered_map<std::wstring, T, string_hash, string_compare>;

	template<typename K, typename V, class Compare = std::less<V>>
	struct map_value_compare {
		Compare m_Compare;
		inline bool operator()(const std::pair<K, V> &a, const std::pair<K, V> &b)
		{
			return m_Compare(a.second, b.second);
		}
	};

	// Removes instances of a character at the beginning and end of the string.
	inline static std::wstring Trim(const std::wstring &str, const wchar_t &character = L' ')
	{
		size_t first = str.find_first_not_of(character);

		if (first == std::wstring::npos)
		{
			return L"";
		}

		size_t last = str.find_last_not_of(character);
		return str.substr(first, (last - first + 1));
	}

	// Removes instances of a character at the beginning and end of the string.
	inline static void TrimInplace(std::wstring &str, const wchar_t &character = L' ')
	{
		size_t first = str.find_first_not_of(character);

		if (first == std::wstring::npos)
		{
			str = L"";
			return;
		}

		size_t last = str.find_last_not_of(character);
		str.erase(last + 1);
		str.erase(0, first);
	}


	// Changes a value. Use with std::bind and context menu callbacks (BindEnum preferred).
	template<typename T>
	inline static void UpdateValue(T &toupdate, T newvalue)
	{
		toupdate = newvalue;
	}

	// Inverts a boolean. Use with std::bind and context menu callbacks (BindBool preferred).
	inline static void InvertBool(bool &value)
	{
		value = !value;
	}

	// Gets a random number from an distribution of numbers.
	template<typename T = int>
	inline static T GetRandomNumber(const T &begin = (std::numeric_limits<T>::min)(), const T &end = (std::numeric_limits<T>::max)())
	{
		std::random_device seed;
		std::mt19937 rng(seed());
		std::uniform_int_distribution<T> distribution(begin, end);

		return distribution(rng);
	}
};