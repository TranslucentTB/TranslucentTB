#pragma once
#include <string_view>
#include <optional>


#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"

struct ActiveInactiveTaskbarAppearance : TaskbarAppearance {
	std::optional<TaskbarAppearance> Inactive;

	ActiveInactiveTaskbarAppearance() = default;
	ActiveInactiveTaskbarAppearance(std::optional<TaskbarAppearance> inactive, ACCENT_STATE accent, Util::Color color, bool showPeek, bool showLine) :
		TaskbarAppearance(accent, color, showPeek, showLine),
		Inactive(std::move(inactive))
	{ }

	template<typename Writer>
	inline void Serialize(Writer &writer) const
	{
		TaskbarAppearance::Serialize(writer);
		if (Inactive.has_value()) {
			rjh::Serialize(writer, Inactive.value(), INACTIVE_KEY);
		}
	}

	inline void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == INACTIVE_KEY)
			{
				TaskbarAppearance appearance;
				rjh::Deserialize(it->value, appearance, key, unknownKeyCallback);
				Inactive.emplace(appearance);
			}
			else
			{
				InnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}

private:
	static constexpr std::wstring_view INACTIVE_KEY = L"inactive";
};
