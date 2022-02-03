#pragma once
#include <format>
#include <string_view>
#include <vector>

#include "optionaltaskbarappearance.hpp"
#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"
#include "../constants.hpp"
#include "../win32.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include "../../TranslucentTB/windows/window.hpp"
#endif

struct RuledTaskbarAppearance : OptionalTaskbarAppearance {
	std::unordered_map<std::wstring, TaskbarAppearance> ClassRules;
	win32::FilenameMap<TaskbarAppearance> FileRules;
	std::unordered_map<std::wstring, TaskbarAppearance> TitleRules;

	RuledTaskbarAppearance() noexcept = default;
	RuledTaskbarAppearance(std::unordered_map<std::wstring, TaskbarAppearance> classRules, win32::FilenameMap<TaskbarAppearance> fileRules, std::unordered_map<std::wstring, TaskbarAppearance> titleRules, bool enabled, ACCENT_STATE accent, Util::Color color, bool showPeek) :
		OptionalTaskbarAppearance(enabled, accent, color, showPeek),
		ClassRules(std::move(classRules)),
		FileRules(std::move(fileRules)),
		TitleRules(std::move(titleRules))
	{ }

	template<typename Writer>
	inline void Serialize(Writer &writer) const
	{
		OptionalTaskbarAppearance::Serialize(writer);
		rjh::WriteKey(writer, RULES_KEY);
		writer.StartObject();
		SerializeRulesMap(writer, ClassRules, CLASS_KEY);
		SerializeRulesMap(writer, FileRules, FILE_KEY);
		SerializeRulesMap(writer, TitleRules, TITLE_KEY);
		writer.EndObject();
	}

	void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == RULES_KEY)
			{
				DeserializeRulesMap(it->value, unknownKeyCallback);
			}
			else
			{
				OptionalInnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}

#ifdef _TRANSLUCENTTB_EXE
	inline std::optional<TaskbarAppearance> FindRule(const Window window) const
	{
		//for (const Rule &rule : Rules)
		//{
		//	// This is the fastest because we do the less string manipulation, so always try it first
		//	if (!rule.WindowClass.empty())
		//	{
		//		if (rule.WindowClass == window.classname())
		//		{
		//			return rule;
		//		}
		//	}

		//	if (!rule.ProcessName.empty())
		//	{
		//		if (rule.ProcessName == window.file()->filename().native())
		//		{
		//			return rule;
		//		}
		//	}

		//	// Do it last because titles can change, so it's less reliable.
		//	if (!rule.WindowTitle.empty())
		//	{
		//		if (rule.WindowTitle == window.title())
		//		{
		//			return rule;
		//		}
		//	}
		//}

		return std::nullopt;
	}
#endif

private:
	template<typename Writer, typename Hash, typename Equal, typename Alloc>
	inline static void SerializeRulesMap(Writer &writer, const std::unordered_map<std::wstring, TaskbarAppearance, Hash, Equal, Alloc> &map, std::wstring_view mapKey)
	{
		rjh::WriteKey(writer, mapKey);
		writer.StartObject();
		for (const auto &[key, value] : map)
		{
			rjh::Serialize(writer, value, key);
		}
		writer.EndObject();
	}

	inline void DeserializeRulesMap(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == CLASS_KEY)
			{
				DeserializeMap(it->value, ClassRules, unknownKeyCallback);
			}
			else if (key == FILE_KEY)
			{
				DeserializeMap(it->value, FileRules, unknownKeyCallback);
			}
			else if (key == TITLE_KEY)
			{
				DeserializeMap(it->value, TitleRules, unknownKeyCallback);
			}
			else
			{
				unknownKeyCallback(key);
			}
		}
	}

	template<typename Hash, typename Equal, typename Alloc>
	inline static void DeserializeMap(const rjh::value_t &obj, std::unordered_map<std::wstring, TaskbarAppearance, Hash, Equal, Alloc> &map, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);

			TaskbarAppearance rule;
			rjh::Deserialize(it->value, rule, key, unknownKeyCallback);

			map[std::wstring(key)] = rule;
		}
	}

	static constexpr std::wstring_view RULES_KEY = L"rules";
};
