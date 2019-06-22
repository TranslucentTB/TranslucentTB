#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string_view>
#include <unordered_map>

#include "rapidjsonhelper.hpp"
#include "../win32.hpp"
#include "undoc/swca.hpp"
#include "util/colors.hpp"
#include "util/others.hpp"

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	COLORREF     Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, Accent, ACCENT_KEY, s_AccentMap);

		writer.String(COLOR_KEY.data(), COLOR_KEY.length());
		writer.String(Util::StringFromColor(win32::SwapColorOrder(Color)));

		writer.String(OPACITY_KEY.data(), OPACITY_KEY.length());
		writer.Uint((Color & 0xFF000000) >> 24);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (!val.IsObject())
		{
			return;
		}

		RapidJSONHelper::Deserialize(val, Accent, ACCENT_KEY, s_AccentMap);

		if (const auto color = val.FindMember(COLOR_KEY.data()); color != val.MemberEnd() && color->value.IsString())
		{
			try
			{
				Color = (Color & 0xFF000000) + win32::SwapColorOrder(Util::ColorFromString({ color->value.GetString(), color->value.GetStringLength() }));
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
	inline static const std::unordered_map<ACCENT_STATE, std::wstring_view> s_AccentMap = {
		{ ACCENT_NORMAL,                     L"normal"  },
		{ ACCENT_ENABLE_GRADIENT,            L"opaque"  },
		{ ACCENT_ENABLE_TRANSPARENTGRADIENT, L"clear"   },
		{ ACCENT_ENABLE_BLURBEHIND,          L"blur"    },
		{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   L"acrylic" }
	};

	static constexpr std::wstring_view ACCENT_KEY = L"accent";
	static constexpr std::wstring_view COLOR_KEY = L"color";
	static constexpr std::wstring_view OPACITY_KEY = L"opacity";
};