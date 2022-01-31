#pragma once
#include "taskbarappearance.hpp"
#include "../constants.hpp"

struct Rule : TaskbarAppearance {
	std::wstring WindowClass;
	std::wstring WindowTitle;
	std::wstring ProcessName;

	constexpr Rule() noexcept = default;
	constexpr Rule(std::wstring window_class, std::wstring window_title, std::wstring window_processName, ACCENT_STATE accent, Util::Color color, bool showPeek) :
		TaskbarAppearance(accent, color, showPeek),
		WindowClass(window_class),
		WindowTitle(window_title),
		ProcessName(window_processName)
	{
	}

	template <typename Writer>
	inline void Serialize(Writer& writer) const
	{
		rjh::Serialize(writer, WindowClass, CLASS_KEY);
		rjh::Serialize(writer, WindowTitle, TITLE_KEY);
		rjh::Serialize(writer, ProcessName, FILE_KEY);
		TaskbarAppearance::Serialize(writer);
	}

	void Deserialize(const rjh::value_t& obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == CLASS_KEY)
			{
				WindowClass = rjh::ValueToStringView(it->value);
			}
			else if (key == TITLE_KEY)
			{
				WindowTitle = rjh::ValueToStringView(it->value);
			}
			else if (key == FILE_KEY)
			{
				ProcessName = rjh::ValueToStringView(it->value);
			}
			else
			{
				InnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}
};
