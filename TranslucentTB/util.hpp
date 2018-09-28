#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cwctype>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>

class Util {

public:
	// Converts a string to its lowercase variant
	inline static void ToLowerInplace(std::wstring &data)
	{
		std::transform(data.begin(), data.end(), data.begin(), std::towlower);
	}

	// Converts a string to its lowercase variant
	inline static std::wstring ToLower(std::wstring data)
	{
		ToLowerInplace(data);
		return data;
	}

	inline static bool IgnoreCaseStringEquals(std::wstring_view l, std::wstring_view r)
	{
		return std::equal(l.begin(), l.end(), r.begin(), r.end(), [](const wchar_t &a, const wchar_t &b) -> bool
		{
			return std::towlower(a) == std::towlower(b);
		});
	}

private:
	struct string_hash {
		inline std::size_t operator()(std::wstring_view k) const
		{
			static const std::hash<std::wstring> hasher;
			return hasher(ToLower(std::wstring(k)));
		}
	};

	struct string_compare {
		inline bool operator()(std::wstring_view l, std::wstring_view r) const
		{
			return IgnoreCaseStringEquals(l, r);
		}
	};

public:
	// Case-insensitive std::unordered_map with string keys.
	template<typename T>
	using string_view_map = std::unordered_map<const std::wstring_view, T, string_hash, string_compare>;

	template<typename K, typename V, class Compare = std::less<V>>
	struct map_value_compare {
	private:
		Compare m_Compare;

	public:
		inline bool operator()(const std::pair<K, V> &a, const std::pair<K, V> &b)
		{
			return m_Compare(a.second, b.second);
		}
	};

	// Removes instances of a character at the beginning and end of the string.
	static constexpr std::wstring_view Trim(std::wstring_view str, const wchar_t &character = L' ')
	{
		size_t first = str.find_first_not_of(character);

		if (first == std::wstring_view::npos)
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
			str.erase();
			return;
		}

		size_t last = str.find_last_not_of(character);
		str.erase(last + 1);
		str.erase(0, first);
	}

	// Checks if a string begins with another string. More efficient than str.find(text) == 0.
	static constexpr bool StringBeginsWith(std::wstring_view string, std::wstring_view text_to_test)
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
	static constexpr std::wstring_view RemovePrefix(std::wstring_view str, std::wstring_view prefix)
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
	inline static void RemovePrefixInplace(std::wstring &str, std::wstring_view prefix)
	{
		if (StringBeginsWith(str, prefix))
		{
			str.erase(0, prefix.length());
		}
	}

	// Changes a value. Use with std::bind and context menu callbacks (BindEnum preferred).
	template<typename T>
	inline static void UpdateValue(T &toupdate, const T &newvalue)
	{
		toupdate = newvalue;
	}

	// Inverts a boolean. Use with std::bind and context menu callbacks (BindBool preferred).
	inline static void InvertBool(bool &value)
	{
		value = !value;
	}

private:
	// Generates a seed valid for a given Mersenne Twister engine.
	// It returns a std::unique_ptr because std::seed_seq is not copyable or movable.
	template<class T>
	inline static std::unique_ptr<std::seed_seq> GenerateSeed()
	{
		static constexpr size_t size = T::state_size;

		int seed_data[size];
		std::random_device r;

		std::generate_n(seed_data, size, std::ref(r));
		return std::make_unique<std::seed_seq>(seed_data, seed_data + size);
	}

	// Gets a static instance of a Mersenne Twister engine. Can't be put directly in
	// GetRandomNumber because every different template instantion will get a different static variable.
	template<class T>
	inline static T &GetRandomEngine()
	{
		static T rng(*GenerateSeed<T>());

		return rng;
	}

public:
	// Gets a random number from an distribution of numbers.
	template<typename T = int>
	inline static T GetRandomNumber(const T &begin = (std::numeric_limits<T>::min)(), const T &end = (std::numeric_limits<T>::max)())
	{
		std::uniform_int_distribution<T> distribution(begin, end);
		return distribution(GetRandomEngine<std::mt19937>());
	}

	template<typename T, typename U>
	static constexpr T ClampTo(const U &value)
	{
		return static_cast<T>(std::clamp<U>(value, (std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)()));
	}

	template<typename T = std::chrono::seconds>
	inline static T GetCurrentTime()
	{
		using namespace std::chrono;
		return duration_cast<T>(steady_clock::now().time_since_epoch());
	}
};