#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <fmt/format.h>
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../util/to_string_view.hpp"
#include "../util/type_traits.hpp"

namespace rj = rapidjson;

namespace rjh {
	using value_t = rj::GenericValue<rj::UTF16LE<>>;

	struct DeserializationError {
		const std::wstring what;
	};

	static constexpr std::array<std::wstring_view, 7> TYPE_NAMES = {
		L"null",
		L"bool",
		L"bool",
		L"object",
		L"array",
		L"string",
		L"number"
	};

	constexpr bool IsType(rj::Type a, rj::Type b) noexcept
	{
		return a == b ||
			(a == rj::Type::kFalseType && b == rj::Type::kTrueType) ||
			(a == rj::Type::kTrueType && b == rj::Type::kFalseType);
	}

	inline void EnsureType(rj::Type expected, rj::Type actual, std::wstring_view obj)
	{
		if (!IsType(expected, actual)) [[unlikely]]
		{
			throw DeserializationError {
				fmt::format(FMT_STRING(L"Expected {} but found {} while deserializing {}"), TYPE_NAMES.at(expected), TYPE_NAMES.at(actual), obj)
			};
		}
	}

	inline void AssertLength(std::wstring_view str)
	{
		assert(str.length() <= std::numeric_limits<rj::SizeType>::max());
	}

	inline std::wstring_view ValueToStringView(const value_t &val)
	{
		assert(val.GetType() == rj::Type::kStringType); // caller should have already ensured

		return { val.GetString(), val.GetStringLength() };
	}

	inline value_t StringViewToValue(std::wstring_view str)
	{
		AssertLength(str);
		return { str.data(), static_cast<rj::SizeType>(str.length()) };
	}

	template<class Writer>
	inline void WriteKey(Writer &writer, std::wstring_view key)
	{
		AssertLength(key);
		writer.Key(key.data(), static_cast<rj::SizeType>(key.length()));
	}

	template<class Writer>
	inline void WriteString(Writer &writer, std::wstring_view str)
	{
		AssertLength(str);
		writer.String(str.data(), static_cast<rj::SizeType>(str.length()));
	}

	// prevent this overload from being picked on stuff implicitly convertible to bool
	template<class Writer, std::same_as<bool> T>
	inline void Serialize(Writer &writer, T value, std::wstring_view key)
	{
		WriteKey(writer, key);
		writer.Bool(value);
	}

	template<class Writer, typename T>
	requires Util::is_convertible_to_wstring_view_v<T>
	inline void Serialize(Writer &writer, const T &value, std::wstring_view key)
	{
		WriteKey(writer, key);
		WriteString(writer, Util::ToStringView(value));
	}

	template<class Writer, class T, std::size_t size>
	requires std::is_enum_v<T>
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key, const std::array<std::wstring_view, size> &arr)
	{
		if (const auto i = static_cast<std::size_t>(member); i < size)
		{
			WriteKey(writer, key);
			WriteString(writer, arr[i]);
		}
	}

	template<class Writer, class T>
	// prevent ambiguous overload errors
	requires (std::is_class_v<T> && !Util::is_convertible_to_wstring_view_v<T> && !Util::is_optional_v<T>)
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key)
	{
		WriteKey(writer, key);
		writer.StartObject();
		member.Serialize(writer);
		writer.EndObject();
	}

	template<class Writer, class T, typename... Args>
	void Serialize(Writer &writer, const std::optional<T> &member, std::wstring_view key, Args &&...args)
	{
		if (member)
		{
			Serialize(writer, *member, key, std::forward<Args>(args)...);
		}
	}

	inline void Deserialize(const value_t &obj, bool &member, std::wstring_view key)
	{
		EnsureType(rj::Type::kFalseType, obj.GetType(), key);

		member = obj.GetBool();
	}

	template<typename T, std::size_t size>
	requires std::is_enum_v<T>
	inline void Deserialize(const value_t &obj, T &member, std::wstring_view key, const std::array<std::wstring_view, size> &arr)
	{
		EnsureType(rj::Type::kStringType, obj.GetType(), key);

		const auto str = ValueToStringView(obj);
		if (const auto it = std::ranges::find(arr.begin(), arr.end(), str); it != arr.end())
		{
			member = static_cast<T>(it - arr.begin());
		}
		else
		{
			throw rjh::DeserializationError {
				fmt::format(FMT_STRING(L"Found invalid enum string \"{}\" while deserializing key \"{}\""), str, key)
			};
		}
	}

	template<class T>
	// prevent ambiguous overload errors
	requires (std::is_class_v<T> && !Util::is_optional_v<T>)
	inline void Deserialize(const value_t &obj, T &member, std::wstring_view key, void (*unknownKeyCallback)(std::wstring_view))
	{
		EnsureType(rj::Type::kObjectType, obj.GetType(), key);

		member.Deserialize(obj, unknownKeyCallback);
	}

	template<typename T, typename... Args>
	void Deserialize(const value_t &obj, std::optional<T> &member, std::wstring_view key, Args &&...args)
	{
		Deserialize(obj, member.emplace(), key, std::forward<Args>(args)...);
	}
}
