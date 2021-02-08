#pragma once
#include "../arch.h"
#include <array>
#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>
#include <windef.h>

#include "rapidjsonhelper.hpp"
#include "../undoc/user32.hpp"
#include "../util/color.hpp"
#include "../util/fmt.hpp"
#include "../util/to_string_view.hpp"

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	Util::Color Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		rjh::Serialize(writer, Accent, ACCENT_KEY, ACCENT_MAP);

		Util::small_wmemory_buffer<9> buf;
		Color.ToString(buf);
		rjh::Serialize(writer, buf, COLOR_KEY);
	}

	void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			InnerDeserialize(rjh::ValueToStringView(it->name), it->value, unknownKeyCallback);
		}
	}

protected:
	void InnerDeserialize(std::wstring_view key, const rjh::value_t &val, void (*unknownKeyCallback)(std::wstring_view))
	{
		if (key == ACCENT_KEY)
		{
			rjh::Deserialize(val, Accent, key, ACCENT_MAP);
		}
		else if (key == COLOR_KEY)
		{
			rjh::EnsureType(rj::Type::kStringType, val.GetType(), key);

			const auto colorStr = rjh::ValueToStringView(val);
			try
			{
				Color = Util::Color::FromString(colorStr);
			}
			catch (...)
			{
				throw rjh::DeserializationError {
					fmt::format(FMT_STRING(L"Found invalid string \"{}\" while deserializing {}"), colorStr, key)
				};
			}
		}
		else if (unknownKeyCallback)
		{
			unknownKeyCallback(key);
		}
	}

private:
	static constexpr std::array<std::wstring_view, 5> ACCENT_MAP = {
		L"normal",
		L"opaque",
		L"clear",
		L"blur",
		L"acrylic"
	};

	static constexpr std::wstring_view ACCENT_KEY = L"accent";
	static constexpr std::wstring_view COLOR_KEY = L"color";
};
