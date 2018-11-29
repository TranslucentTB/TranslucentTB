#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <cwctype>
#include <initializer_list>
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
		return std::equal(l.begin(), l.end(), r.begin(), r.end(), [](wchar_t a, wchar_t b) -> bool
		{
			return std::towlower(a) == std::towlower(b);
		});
	}

private:
	struct string_hash {
		inline std::size_t operator()(std::wstring_view k) const noexcept
		{
			static const std::hash<std::wstring> hasher;
			return hasher(ToLower(std::wstring(k)));
		}
	};

	struct string_compare {
		inline bool operator()(std::wstring_view l, std::wstring_view r) const noexcept
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
		static constexpr Compare s_Comparer{};

	public:
		constexpr bool operator()(const std::pair<K, V> &a, const std::pair<K, V> &b) const noexcept
		{
			return s_Comparer(a.second, b.second);
		}
	};

	// Removes instances of a character at the beginning and end of the string.
	template<class T = std::wstring_view>
	static constexpr T Trim(const T &str, wchar_t character = L' ')
	{
		size_t first = str.find_first_not_of(character);

		if (first == T::npos)
		{
			return L"";
		}

		size_t last = str.find_last_not_of(character);
		return str.substr(first, (last - first + 1));
	}

	// Removes instances of a character at the beginning and end of the string.
	inline static void TrimInplace(std::wstring &str, wchar_t character = L' ')
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
	static constexpr bool StringBeginsWith(std::wstring_view string, std::wstring_view string_to_test)
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
	static constexpr bool StringBeginsWithOneOf(std::wstring_view string, const T &strings_to_test)
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
	// Correctly seeds and creates a Mersenne Twister engine.
	template<class T>
	inline static T CreateRandomEngine()
	{
		int seed_data[T::state_size];
		std::random_device r;

		std::generate_n(seed_data, T::state_size, std::ref(r));
		std::seed_seq seed(seed_data, seed_data + T::state_size);
		return T(seed);
	}

	// Gets a thread local and static instance of a Mersenne Twister engine. Can't be put directly in
	// GetRandomNumber because every different template instantion will get a different static variable.
	template<class T>
	inline static T &GetRandomEngine()
	{
		static T rng = CreateRandomEngine<T>();

		return rng;
	}

public:
	// Gets a random number from an distribution of numbers.
	template<typename T = int>
	inline static T GetRandomNumber(T begin = (std::numeric_limits<T>::min)(), T end = (std::numeric_limits<T>::max)())
	{
		std::uniform_int_distribution<T> distribution(begin, end);
		return distribution(GetRandomEngine<std::mt19937>());
	}

	// Clamps a numeric type to a narrower numeric type.
	template<typename T, typename U>
	static constexpr T ClampTo(U value)
	{
		return static_cast<T>(std::clamp<U>(value, (std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)()));
	}

	// Gets the number of T elapsed since the Unix epoch
	template<typename T = std::chrono::seconds>
	inline static T GetTime()
	{
		using namespace std::chrono;
		return duration_cast<T>(system_clock::now().time_since_epoch());
	}

	// Gets the time elapsed since the Unix epoch for use with C APIs
	template<>
	inline std::time_t GetTime<std::time_t>()
	{
		using namespace std::chrono;
		return system_clock::to_time_t(system_clock::now());
	}
};