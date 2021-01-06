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
		RapidJSONHelper::Serialize(writer, Accent, ACCENT_KEY, ACCENT_MAP);

		RapidJSONHelper::WriteKey(writer, COLOR_KEY);
		Util::small_wmemory_buffer<9> buf;
		Color.ToString(buf);
		RapidJSONHelper::WriteString(writer, Util::ToStringView(buf));
	}

	void Deserialize(const RapidJSONHelper::value_t &obj)
	{
		RapidJSONHelper::Deserialize(obj, Accent, ACCENT_KEY, ACCENT_MAP);

		if (const auto it = obj.FindMember(RapidJSONHelper::StringViewToValue(COLOR_KEY)); it != obj.MemberEnd())
		{
			const auto &color = it->value;
			RapidJSONHelper::EnsureType(rapidjson::Type::kStringType, color.GetType(), COLOR_KEY);

			const auto colorStr = RapidJSONHelper::ValueToStringView(color);
			try
			{
				Color = Util::Color::FromString(colorStr);
			}
			catch (...)
			{
				throw RapidJSONHelper::DeserializationError {
					fmt::format(FMT_STRING(L"Found invalid string \"{}\" while deserializing {}"), colorStr, COLOR_KEY)
				};
			}
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
