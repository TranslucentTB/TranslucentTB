#pragma once
#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class Util {

public:
	// Converts a string to it's lowercase variant
	inline static void ToLower(std::wstring &data)
	{
		std::transform(data.begin(), data.end(), data.begin(), std::towlower);
	}

private:
	struct string_hash {
		std::hash<std::wstring> m_Hasher;
		inline std::size_t operator()(std::wstring k) const
		{
			ToLower(k);
			return m_Hasher(k);
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

	// Removes instances of character at the beginning and end of the string.
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

	// Changes a value. Use with std::bind and context menu callbacks (BindEnum preferred).
	template<typename T>
	inline static void UpdateValue(T &toupdate, T newvalue, ...)
	{
		toupdate = newvalue;
	}

	// Inverts a boolean. Use with std::bind and context menu callbacks (BindBool preferred).
	inline static void InvertBool(bool &value, ...)
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

	// Copies text to the clipboard.
	static void CopyToClipboard(const std::wstring &text);

	// Opens a file in the default text editor.
	static void EditFile(const std::wstring &file);

	// Opens a link in the default browser.
	// NOTE: doesn't attempts to validate the link, make sure it's correct.
	static void OpenLink(const std::wstring &link);

	// Opens a color picker.
	// NOTE: use .join() to wait for input, because this doesn't blocks by default.
	static std::thread PickColor(uint32_t &color);

	// Checks if the start menu is open using a COM interface.
	static bool IsStartVisible();
};