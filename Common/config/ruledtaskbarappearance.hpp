#pragma once
#include <format>
#include <string_view>
#include <vector>

#include "optionaltaskbarappearance.hpp"
#include "activeinactivetaskbarappearance.hpp"
#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"
#include "../constants.hpp"
#include "../win32.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include "../../TranslucentTB/windows/window.hpp"
#include "../../ProgramLog/error/std.hpp"
#endif

struct RuledTaskbarAppearance : OptionalTaskbarAppearance {
	std::unordered_map<std::wstring, ActiveInactiveTaskbarAppearance> ClassRules;
	std::unordered_map<std::wstring, ActiveInactiveTaskbarAppearance> TitleRules;
	win32::FilenameMap<ActiveInactiveTaskbarAppearance> FileRules;

	RuledTaskbarAppearance() = default;
	RuledTaskbarAppearance(std::unordered_map<std::wstring, ActiveInactiveTaskbarAppearance> classRules, std::unordered_map<std::wstring, ActiveInactiveTaskbarAppearance> titleRules, win32::FilenameMap<ActiveInactiveTaskbarAppearance> fileRules, bool enabled, ACCENT_STATE accent, Util::Color color, bool showPeek, bool showLine) :
		OptionalTaskbarAppearance(enabled, accent, color, showPeek, showLine),
		ClassRules(std::move(classRules)),
		TitleRules(std::move(titleRules)),
		FileRules(std::move(fileRules))
	{ }

	template<typename Writer>
	inline void Serialize(Writer &writer) const
	{
		OptionalTaskbarAppearance::Serialize(writer);
		rjh::WriteKey(writer, RULES_KEY);
		writer.StartObject();
		SerializeRulesMap(writer, ClassRules, CLASS_KEY);
		SerializeRulesMap(writer, TitleRules, TITLE_KEY);
		SerializeRulesMap(writer, FileRules, FILE_KEY);
		writer.EndObject();
	}

	inline void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
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
		const auto rule = FindRuleInner(window);
		if (window.active())
		{
			return rule;
		}
		else
		{
			return rule.Inactive;
		}
	}

	inline std::optional<TaskbarAppearance> FindRuleInner(const Window window) const
	{
		// This is the fastest because we do the less string manipulation, so always try it first
		if (!ClassRules.empty())
		{
			if (const auto className = window.classname())
			{
				if (const auto it = ClassRules.find(*className); it != ClassRules.end())
				{
					return it->second;
				}
			}
			else
			{
				return std::nullopt;
			}
		}

		if (!FileRules.empty())
		{
			if (const auto file = window.file())
			{
				try
				{
					if (const auto it = FileRules.find(file->filename().native()); it != FileRules.end())
					{
						return it->second;
					}
				}
				StdSystemErrorCatch(spdlog::level::warn, L"Failed to check if window process is part of window filter");
			}
			else
			{
				return std::nullopt;
			}
		}

		// Do it last because titles can change, so it's less reliable.
		if (!TitleRules.empty())
		{
			if (const auto title = window.title())
			{
				for (const auto &[key, value] : TitleRules)
				{
					if (title->find(key) != std::wstring::npos)
					{
						return value;
					}
				}
			}
		}

		return std::nullopt;
	}
#endif

	inline bool HasRules() const noexcept
	{
		return !(ClassRules.empty() && FileRules.empty() && TitleRules.empty());
	}

private:
	template<typename Writer, typename Hash, typename Equal, typename Alloc>
	inline static void SerializeRulesMap(Writer &writer, const std::unordered_map<std::wstring, ActiveInactiveTaskbarAppearance, Hash, Equal, Alloc> &map, std::wstring_view mapKey)
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
			else if (key == TITLE_KEY)
			{
				DeserializeMap(it->value, TitleRules, unknownKeyCallback);
			}
			else if (key == FILE_KEY)
			{
				DeserializeMap(it->value, FileRules, unknownKeyCallback);
			}
			else
			{
				unknownKeyCallback(key);
			}
		}
	}

	template<typename Hash, typename Equal, typename Alloc>
	inline static void DeserializeMap(const rjh::value_t &obj, std::unordered_map<std::wstring, ActiveInactiveTaskbarAppearance, Hash, Equal, Alloc> &map, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);

			ActiveInactiveTaskbarAppearance rule;
			rjh::Deserialize(it->value, rule, key, unknownKeyCallback);

			map[std::wstring(key)] = rule;
		}
	}

	static constexpr std::wstring_view RULES_KEY = L"rules";
};
