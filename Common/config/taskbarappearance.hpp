#pragma once
#include "../arch.h"
#include <array>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>
#include <windef.h>

#include "rapidjsonhelper.hpp"
#include "../undoc/user32.hpp"
#include "../util/colors.hpp"
#include "../util/null_terminated_string_view.hpp"

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	COLORREF     Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, Accent, ACCENT_KEY, ACCENT_MAP);

		writer.String(COLOR_KEY.data(), static_cast<rapidjson::SizeType>(COLOR_KEY.length()));
		writer.String(Util::StringFromColor(Util::SwapColorEndian(Color)));

		writer.String(OPACITY_KEY.data(), static_cast<rapidjson::SizeType>(OPACITY_KEY.length()));
		writer.Uint((Color & 0xFF000000) >> 24);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (!val.IsObject())
		{
			return;
		}

		RapidJSONHelper::Deserialize(val, Accent, ACCENT_KEY, ACCENT_MAP);

		if (const auto color = val.FindMember(COLOR_KEY.data()); color != val.MemberEnd() && color->value.IsString())
		{
			try
			{
				Color = (Color & 0xFF000000) + Util::SwapColorEndian(Util::ColorFromString({ color->value.GetString(), color->value.GetStringLength() }));
			}
			catch (const std::exception &)
			{
				// ignore
			}
		}

		if (const auto opacity = val.FindMember(OPACITY_KEY.data()); opacity != val.MemberEnd() && opacity->value.IsInt())
		{
			Color = (std::clamp(opacity->value.GetInt(), 0, 255) << 24) + (Color & 0xFFFFFF);
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

	static constexpr Util::null_terminated_wstring_view ACCENT_KEY = L"accent";
	static constexpr Util::null_terminated_wstring_view COLOR_KEY = L"color";
	static constexpr Util::null_terminated_wstring_view OPACITY_KEY = L"opacity";
};
