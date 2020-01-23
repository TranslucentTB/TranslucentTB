#pragma once
#include <algorithm>
#include <array>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>

#include "../util/fmt.hpp"
#include "../util/null_terminated_string_view.hpp"
#include "../util/map.hpp"

namespace RapidJSONHelper {
	struct DeserializationError {
		const std::wstring what;
	};

	inline void EnsureType(rapidjson::Type expected, rapidjson::Type actual, std::wstring_view obj)
	{
		static constexpr std::array<std::wstring_view, 7> TYPE_NAMES = {
			L"null",
			L"bool",
			L"bool",
			L"object",
			L"array",
			L"string",
			L"number"
		};

		if (expected == rapidjson::Type::kTrueType)
		{
			expected = rapidjson::Type::kFalseType;
		}

		if (actual == rapidjson::Type::kTrueType)
		{
			actual = rapidjson::Type::kFalseType;
		}

		if (actual != expected)
		{
			Util::small_wmemory_buffer<100> buf;
			fmt::format_to(buf, fmt(L"Expected {} but found {} while deserializing key \"{}\""), TYPE_NAMES[expected], TYPE_NAMES[actual], obj);

			throw DeserializationError { fmt::to_string(buf) };
		}
	}

	template<class Writer>
	inline void Serialize(Writer &writer, bool value, std::wstring_view key)
	{
		writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
		writer.Bool(value);
	}

	template<class Writer, class T, std::size_t size>
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key, const std::array<std::wstring_view, size> &arr)
	{
		if (member >= 0 && member <= size - 1)
		{
			writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));

			const auto enum_str = arr[member];
			writer.String(enum_str.data(), static_cast<rapidjson::SizeType>(enum_str.length()));
		}
	}

	template<class Writer, class T>
	inline void Serialize(Writer &writer, const T &member, std::wstring_view key)
	{
		writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
		writer.StartObject();
		member.Serialize(writer);
		writer.EndObject();
	}

	// TODO: test
	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj, bool &member, Util::null_terminated_wstring_view key)
	{
		if (const auto it = obj.FindMember(key.c_str()); it != obj.MemberEnd())
		{
			const auto &value = it->value;
			EnsureType(rapidjson::Type::kFalseType, value.GetType(), key);

			member = value.GetBool();
		}
	}

	template<class T, std::size_t size>
	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj, T &member, Util::null_terminated_wstring_view key, const std::array<std::wstring_view, size> &arr)
	{
		if (const auto it = obj.FindMember(key.c_str()); it != obj.MemberEnd())
		{
			const auto &value = it->value;
			EnsureType(rapidjson::Type::kStringType, value.GetType(), key);

			if (const auto it2 = std::find(arr.begin(), arr.end(), std::wstring_view(value.GetString(), value.GetStringLength()));
				it2 != arr.end())
			{
				member = static_cast<T>(it2 - arr.begin());
			}
			else
			{
				// TODO: throw
			}
		}
	}

	template<class T>
	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj, T &member, Util::null_terminated_wstring_view key)
	{
		if (const auto it = obj.FindMember(key.c_str()); it != obj.MemberEnd())
		{
			const auto &value = it->value;
			EnsureType(rapidjson::Type::kObjectType, value.GetType(), key);

			member.Deserialize(value);
		}
	}
}
