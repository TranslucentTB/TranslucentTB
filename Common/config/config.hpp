#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <spdlog/common.h>
#include <string_view>
#include <unordered_map>

#include "optionaltaskbarappearance.hpp"
#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"
#include "windowmatchlist.hpp"

enum class PeekBehavior {
	AlwaysHide,                   // Always hide the button
	WindowMaximisedOnMainMonitor, // Show when a window is maximised on the main monitor
	WindowMaximisedOnAnyMonitor,  // Show when a window is maximised on any monitor
	DesktopIsForegroundWindow,    // Show when the desktop is the foreground window
	AlwaysShow                    // Always show the button
};

class Config {
public:
	// Appearances
	TaskbarAppearance DesktopAppearance;
	OptionalTaskbarAppearance VisibleWindowAppearance;
	OptionalTaskbarAppearance MaximisedWindowAppearance;
	OptionalTaskbarAppearance StartOpenedAppearance;
	OptionalTaskbarAppearance CortanaOpenedAppearance;
	OptionalTaskbarAppearance TimelineOpenedAppearance;

	// Peek
	PeekBehavior Peek;
	bool UseRegularAppearanceWhenPeeking;

	// Advanced
	WindowMatchList Whitelist;
	WindowMatchList Blacklist;
	bool HideTray;
	bool DisableSaving;
	spdlog::level::level_enum LogVerbosity;

	// Default-init with default settings
	inline Config() noexcept :
		DesktopAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0 },
		VisibleWindowAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0, false },
		MaximisedWindowAppearance { ACCENT_ENABLE_BLURBEHIND, 0xaa000000, true },
		StartOpenedAppearance { ACCENT_NORMAL, 0, true },
		CortanaOpenedAppearance { ACCENT_NORMAL, 0, true },
		TimelineOpenedAppearance { ACCENT_NORMAL, 0, true },
		Peek(PeekBehavior::WindowMaximisedOnMainMonitor),
		UseRegularAppearanceWhenPeeking(true),
		Whitelist(),
		Blacklist(),
		HideTray(false),
		DisableSaving(false),
#ifdef _DEBUG
		LogVerbosity(spdlog::level::debug)
#else
		LogVerbosity(spdlog::level::warn)
#endif
	{ }

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		RapidJSONHelper::Serialize(writer, DesktopAppearance, DESKTOP_KEY);
		RapidJSONHelper::Serialize(writer, VisibleWindowAppearance, VISIBLE_KEY);
		RapidJSONHelper::Serialize(writer, MaximisedWindowAppearance, MAXIMISED_KEY);
		RapidJSONHelper::Serialize(writer, StartOpenedAppearance, START_KEY);
		RapidJSONHelper::Serialize(writer, CortanaOpenedAppearance, CORTANA_KEY);
		RapidJSONHelper::Serialize(writer, TimelineOpenedAppearance, TIMELINE_KEY);
		RapidJSONHelper::Serialize(writer, Peek, PEEK_KEY, s_PeekMap);
		RapidJSONHelper::Serialize(writer, UseRegularAppearanceWhenPeeking, REGULAR_ON_PEEK_KEY);
		RapidJSONHelper::Serialize(writer, Whitelist, WHITELIST_KEY);
		RapidJSONHelper::Serialize(writer, Blacklist, BLACKLIST_KEY);
		RapidJSONHelper::Serialize(writer, HideTray, TRAY_KEY);
		RapidJSONHelper::Serialize(writer, DisableSaving, SAVING_KEY);
		RapidJSONHelper::Serialize(writer, LogVerbosity, LOG_KEY, s_LogMap);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
	{
		if (!val.IsObject())
		{
			return;
		}

		RapidJSONHelper::Deserialize(val, DesktopAppearance, DESKTOP_KEY);
		RapidJSONHelper::Deserialize(val, VisibleWindowAppearance, VISIBLE_KEY);
		RapidJSONHelper::Deserialize(val, MaximisedWindowAppearance, MAXIMISED_KEY);
		RapidJSONHelper::Deserialize(val, StartOpenedAppearance, START_KEY);
		RapidJSONHelper::Deserialize(val, CortanaOpenedAppearance, CORTANA_KEY);
		RapidJSONHelper::Deserialize(val, TimelineOpenedAppearance, TIMELINE_KEY);
		RapidJSONHelper::Deserialize(val, Peek, PEEK_KEY, s_PeekMap);
		RapidJSONHelper::Deserialize(val, UseRegularAppearanceWhenPeeking, REGULAR_ON_PEEK_KEY);
		RapidJSONHelper::Deserialize(val, Whitelist, WHITELIST_KEY);
		RapidJSONHelper::Deserialize(val, Blacklist, BLACKLIST_KEY);
		RapidJSONHelper::Deserialize(val, HideTray, TRAY_KEY);
		RapidJSONHelper::Deserialize(val, DisableSaving, SAVING_KEY);
		RapidJSONHelper::Deserialize(val, LogVerbosity, LOG_KEY, s_LogMap);
	}

private:
	inline static const std::unordered_map<PeekBehavior, std::wstring_view> s_PeekMap = {
		{ PeekBehavior::AlwaysHide,                   L"never"                                 },
		{ PeekBehavior::WindowMaximisedOnMainMonitor, L"when_maximised_window_on_main_monitor" },
		{ PeekBehavior::WindowMaximisedOnAnyMonitor,  L"when_maximised_window_on_any_monitor"  },
		{ PeekBehavior::DesktopIsForegroundWindow,    L"when_desktop_is_foreground_window"     },
		{ PeekBehavior::AlwaysShow,                   L"always"                                }
	};

	inline static const std::unordered_map<spdlog::level::level_enum, std::wstring_view> s_LogMap = {
		{ spdlog::level::debug, L"debug"   },
		{ spdlog::level::info,  L"info"    },
		{ spdlog::level::warn,  L"warning" },
		{ spdlog::level::err,   L"error"   },
		{ spdlog::level::off,   L"off"     }
	};

	static constexpr std::wstring_view DESKTOP_KEY = L"desktop_appearance";
	static constexpr std::wstring_view VISIBLE_KEY = L"visible_window_appearance";
	static constexpr std::wstring_view MAXIMISED_KEY = L"maximised_window_appearance";
	static constexpr std::wstring_view START_KEY = L"start_opened_appearance";
	static constexpr std::wstring_view CORTANA_KEY = L"cortana_opened_appearance";
	static constexpr std::wstring_view TIMELINE_KEY = L"timeline_opened_appearance";
	static constexpr std::wstring_view PEEK_KEY = L"show_peek_button";
	static constexpr std::wstring_view REGULAR_ON_PEEK_KEY = L"regular_appearance_when_peeking";
	static constexpr std::wstring_view WHITELIST_KEY = L"whitelist";
	static constexpr std::wstring_view BLACKLIST_KEY = L"blacklist";
	static constexpr std::wstring_view TRAY_KEY = L"hide_tray";
	static constexpr std::wstring_view SAVING_KEY = L"disable_saving";
	static constexpr std::wstring_view LOG_KEY = L"verbosity";
};