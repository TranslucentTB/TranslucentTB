#pragma once
#include <array>
#include <spdlog/common.h>
#include <string_view>
#include <optional>

#include "optionaltaskbarappearance.hpp"
#include "rapidjsonhelper.hpp"
#include "ruledtaskbarappearance.hpp"
#include "taskbarappearance.hpp"
#include "../win32.hpp"
#include "windowfilter.hpp"

class Config {
private:
	inline static bool IsWindows11() noexcept
	{
		static const bool isWindows11 = win32::IsAtLeastBuild(22000);
		return isWindows11;
	}

public:
	static constexpr spdlog::level::level_enum DEFAULT_LOG_VERBOSITY =
#ifdef _DEBUG
		spdlog::level::debug;
#else
		spdlog::level::warn;
#endif

	// Appearances
	TaskbarAppearance DesktopAppearance = { ACCENT_ENABLE_TRANSPARENTGRADIENT, { 0, 0, 0, 0 }, false };
	RuledTaskbarAppearance VisibleWindowAppearance = { {}, {}, {}, false, ACCENT_ENABLE_TRANSPARENTGRADIENT, { 0, 0, 0, 0 }, true };
	RuledTaskbarAppearance MaximisedWindowAppearance = IsWindows11()
		? RuledTaskbarAppearance { {}, {}, {}, false, ACCENT_ENABLE_ACRYLICBLURBEHIND, { 0, 0, 0, 0 }, true }
		: RuledTaskbarAppearance { {}, {}, {}, true, ACCENT_ENABLE_BLURBEHIND, { 0, 0, 0, 0 }, true };
	OptionalTaskbarAppearance StartOpenedAppearance = { !IsWindows11(), ACCENT_NORMAL, { 0, 0, 0, 0 }, true };
	OptionalTaskbarAppearance SearchOpenedAppearance = { !IsWindows11(), ACCENT_NORMAL, { 0, 0, 0, 0 }, true };
	OptionalTaskbarAppearance TaskViewOpenedAppearance = { true, ACCENT_NORMAL, { 0, 0, 0, 0 }, false };
	OptionalTaskbarAppearance BatterySaverAppearance = { false, ACCENT_ENABLE_GRADIENT, { 0, 0, 0, 0 }, true };

	// Advanced
	WindowFilter IgnoredWindows;
	bool HideTray = false;
	bool DisableSaving = false;
	spdlog::level::level_enum LogVerbosity = DEFAULT_LOG_VERBOSITY;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		rjh::Serialize(writer, DesktopAppearance, DESKTOP_KEY);
		rjh::Serialize(writer, VisibleWindowAppearance, VISIBLE_KEY);
		rjh::Serialize(writer, MaximisedWindowAppearance, MAXIMISED_KEY);
		rjh::Serialize(writer, StartOpenedAppearance, START_KEY);
		rjh::Serialize(writer, SearchOpenedAppearance, SEARCH_KEY);
		rjh::Serialize(writer, TaskViewOpenedAppearance, TASKVIEW_KEY);
		rjh::Serialize(writer, BatterySaverAppearance, BATTERYSAVER_KEY);
		rjh::Serialize(writer, IgnoredWindows, IGNORED_WINDOWS_KEY);
		rjh::Serialize(writer, HideTray, TRAY_KEY);
		rjh::Serialize(writer, DisableSaving, SAVING_KEY);
		rjh::Serialize(writer, LogVerbosity, LOG_KEY, LOG_MAP);
	}

	inline void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view) = nullptr)
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == DESKTOP_KEY)
			{
				rjh::Deserialize(it->value, DesktopAppearance, key, unknownKeyCallback);
			}
			else if (key == VISIBLE_KEY)
			{
				rjh::Deserialize(it->value, VisibleWindowAppearance, key, unknownKeyCallback);
			}
			else if (key == MAXIMISED_KEY)
			{
				rjh::Deserialize(it->value, MaximisedWindowAppearance, key, unknownKeyCallback);
			}
			else if (key == START_KEY)
			{
				rjh::Deserialize(it->value, StartOpenedAppearance, key, unknownKeyCallback);
			}
			else if (key == SEARCH_KEY)
			{
				rjh::Deserialize(it->value, SearchOpenedAppearance, key, unknownKeyCallback);
			}
			else if (key == TASKVIEW_KEY)
			{
				rjh::Deserialize(it->value, TaskViewOpenedAppearance, key, unknownKeyCallback);
			}
			else if (key == BATTERYSAVER_KEY)
			{
				rjh::Deserialize(it->value, BatterySaverAppearance, key, unknownKeyCallback);
			}
			else if (key == IGNORED_WINDOWS_KEY)
			{
				rjh::Deserialize(it->value, IgnoredWindows, key, unknownKeyCallback);
			}
			else if (key == TRAY_KEY)
			{
				rjh::Deserialize(it->value, HideTray, key);
			}
			else if (key == SAVING_KEY)
			{
				rjh::Deserialize(it->value, DisableSaving, key);
			}
			else if (key == LOG_KEY)
			{
				rjh::Deserialize(it->value, LogVerbosity, key, LOG_MAP);
			}
			else if (unknownKeyCallback)
			{
				unknownKeyCallback(key);
			}
		}
	}

private:
	static constexpr std::array<std::wstring_view, spdlog::level::n_levels> LOG_MAP = {
		L"trace",
		L"debug",
		L"info",
		L"warn",
		L"err",
		L"critical",
		L"off"
	};

	static constexpr std::wstring_view DESKTOP_KEY = L"desktop_appearance";
	static constexpr std::wstring_view VISIBLE_KEY = L"visible_window_appearance";
	static constexpr std::wstring_view MAXIMISED_KEY = L"maximized_window_appearance";
	static constexpr std::wstring_view START_KEY = L"start_opened_appearance";
	static constexpr std::wstring_view SEARCH_KEY = L"search_opened_appearance";
	static constexpr std::wstring_view TASKVIEW_KEY = L"task_view_opened_appearance";
	static constexpr std::wstring_view BATTERYSAVER_KEY = L"battery_saver_appearance";
	static constexpr std::wstring_view IGNORED_WINDOWS_KEY = L"ignored_windows";
	static constexpr std::wstring_view TRAY_KEY = L"hide_tray";
	static constexpr std::wstring_view SAVING_KEY = L"disable_saving";
	static constexpr std::wstring_view LOG_KEY = L"verbosity";
};
