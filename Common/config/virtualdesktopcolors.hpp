#pragma once
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <objbase.h>
#include "taskbarappearance.hpp"

struct VirtualDesktopColors {
	bool Enabled = false;
	std::map<std::wstring, Util::Color> Colors;

	static std::wstring Key(const GUID &id)
	{
		wchar_t text[39]{};
		StringFromGUID2(id, text, _countof(text));
		return text;
	}

	std::optional<Util::Color> Find(const GUID &id) const
	{
		if (Enabled)
		{
			if (const auto it = Colors.find(Key(id)); it != Colors.end())
			{
				return it->second;
			}
		}
		return std::nullopt;
	}

	static TaskbarAppearance Apply(TaskbarAppearance appearance, const std::optional<Util::Color> &color) noexcept
	{
		if (color)
		{
			appearance.Color = *color;
			// Normal delegates rendering to Windows, which ignores our color.
			if (appearance.Accent == ACCENT_NORMAL)
			{
				appearance.Accent = ACCENT_ENABLE_GRADIENT;
			}
		}
		return appearance;
	}

#ifdef HAS_RAPIDJSON
	template<class Writer>
	void Serialize(Writer &writer) const
	{
		rjh::Serialize(writer, Enabled, L"enabled");
		rjh::WriteKey(writer, L"colors");
		writer.StartObject();
		for (const auto &[id, color] : Colors)
		{
			rjh::Serialize(writer, color.ToString(), id);
		}
		writer.EndObject();
	}

	void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::kObjectType, obj.GetType(), L"virtual_desktop_colors");
		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			const auto key = rjh::ValueToStringView(it->name);
			if (key == L"enabled")
			{
				rjh::Deserialize(it->value, Enabled, key);
			}
			else if (key == L"colors")
			{
				rjh::EnsureType(rj::kObjectType, it->value.GetType(), key);
				std::map<std::wstring, Util::Color> colors;
				for (auto entry = it->value.MemberBegin(); entry != it->value.MemberEnd(); ++entry)
				{
					const std::wstring id(rjh::ValueToStringView(entry->name));
					GUID guid{};
					if (id.size() != 38 || id.front() != L'{' || id.back() != L'}' || FAILED(CLSIDFromString(id.c_str(), &guid)) || IsEqualGUID(guid, GUID_NULL))
					{
						throw rjh::DeserializationError { L"Invalid virtual desktop GUID: " + id };
					}
					rjh::EnsureType(rj::kStringType, entry->value.GetType(), id);
					Util::Color color;
					try
					{
						color = Util::Color::FromString(rjh::ValueToStringView(entry->value));
					}
					catch (...)
					{
						throw rjh::DeserializationError { L"Invalid virtual desktop color: " + id };
					}
					if (!colors.emplace(Key(guid), color).second)
					{
						throw rjh::DeserializationError { L"Duplicate virtual desktop GUID: " + id };
					}
				}
				Colors = std::move(colors);
			}
			else if (unknownKeyCallback)
			{
				unknownKeyCallback(key);
			}
		}
	}
#endif
};
