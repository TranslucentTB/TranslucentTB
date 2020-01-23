#pragma once
#include "../util/null_terminated_string_view.hpp"
#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"

struct OptionalTaskbarAppearance : TaskbarAppearance {
	bool Enabled;

	template <typename Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, Enabled, ENABLED_KEY);
		TaskbarAppearance::Serialize(writer);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj)
	{
		RapidJSONHelper::Deserialize(obj, Enabled, ENABLED_KEY);
		TaskbarAppearance::Deserialize(obj);
	}

private:
	static constexpr Util::null_terminated_wstring_view ENABLED_KEY = L"enabled";
};
