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

	template<size_t s>
	inline static bool IgnoreCaseStringEquals(const std::wstring &l, const wchar_t(&r)[s])
	{
		return std::equal(l.begin(), l.end(), r, r + s, [](const wchar_t &a, const wchar_t &b) -> bool
		{
			return b != L'\0' && std::towlower(a) == std::towlower(b);
		});
	}

	inline static bool IgnoreCaseStringEquals(const std::wstring &l, const std::wstring &r)
	{
		return std::equal(l.begin(), l.end(), r.begin(), r.end(), [](const wchar_t &a, const wchar_t &b) -> bool
		{
			return std::towlower(a) == std::towlower(b);
		});
	}

private:
	class string_hash {

	private:
		std::hash<std::wstring> m_Hasher;

	public:
		inline std::size_t operator()(const std::wstring &k) const
		{
			return m_Hasher(ToLower(k));
		}
	};

	struct string_compare {
		inline bool operator()(const std::wstring &l, const std::wstring &r) const
		{
			return IgnoreCaseStringEquals(l, r);
		}
	};

public:
	// Case-insensitive std::unordered_map with string keys.
	template<typename T>
	using string_map = std::unordered_map<std::wstring, T, string_hash, string_compare>;

	template<typename K, typename V, class Compare = std::less<V>>
	class map_value_compare {
	private:
		Compare m_Compare;

	public:
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

	// Checks if a string begins with another string. More efficient than str.find(text) == 0.
	inline static bool StringBeginsWith(const std::wstring &string, const std::wstring &text_to_test)
	{
		const size_t length = text_to_test.length();
		if (string.length() < length)
		{
			return false;
		}

		for (size_t i = 0; i < length; i++)
		{
			if (text_to_test[i] != string[i])
			{
				return false;
			}
		}
		return true;
	}

	// Removes a string at the beginning of another string.
	inline static std::wstring RemovePrefix(const std::wstring &str, const std::wstring &prefix)
	{
		if (StringBeginsWith(str, prefix))
		{
			return str.substr(prefix.length());
		}
		else
		{
			return str;
		}
	}

	// Removes a string at the beginning of another string.
	inline static void RemovePrefixInplace(std::wstring &str, const std::wstring &prefix)
	{
		if (StringBeginsWith(str, prefix))
		{
			str.erase(0, prefix.length());
		}
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

private:
	// Gets a static instance of a Mersenne Twister engine. Can't be put directly in
	// GetRandomNumber because every different template instantion will get a different static variable.
	inline static std::mt19937 &GetRandomEngine()
	{
		static std::mt19937 rng(std::random_device{}());
		return rng;
	}

public:
	// Gets a random number from an distribution of numbers.
	template<typename T = int>
	inline static T GetRandomNumber(const T &begin = (std::numeric_limits<T>::min)(), const T &end = (std::numeric_limits<T>::max)())
	{
		std::uniform_int_distribution<T> distribution(begin, end);
		return distribution(GetRandomEngine());
	}
};