#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <string>
#include <string_view>
#include <unordered_map>

#include "blacklist.hpp"
#include "win32.hpp"
#include "undoc/swca.hpp"
#include "util/colors.hpp"
#include "util/others.hpp"

#pragma warning(push)
#pragma warning(disable: 4267)

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	COLORREF     Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(ACCENT_KEY.data(), ACCENT_KEY.length());
		const auto accent_str = Util::FindOrDefault(s_AccentMap, Accent);
		writer.String(accent_str.data(), accent_str.length());

		writer.String(COLOR_KEY.data(), COLOR_KEY.length());
		writer.String(Util::StringFromColor(win32::SwapColorOrder(Color)));

		writer.String(OPACITY_KEY.data(), OPACITY_KEY.length());
		writer.Uint((Color & 0xFF000000) >> 24);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val);

private:
	static const std::unordered_map<ACCENT_STATE, std::wstring_view> s_AccentMap;

	static constexpr std::wstring_view ACCENT_KEY = L"accent";
	static constexpr std::wstring_view COLOR_KEY = L"color";
	static constexpr std::wstring_view OPACITY_KEY = L"opacity";
};

struct OptionalTaskbarAppearance : TaskbarAppearance {
	bool Enabled;

	template <typename Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(ENABLED_KEY.data(), ENABLED_KEY.length());
		writer.Bool(Enabled);

		TaskbarAppearance::Serialize(writer);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val);

private:
	static constexpr std::wstring_view ENABLED_KEY = L"enabled";
};

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
	TaskbarAppearance RegularAppearance;
	OptionalTaskbarAppearance MaximisedWindowAppearance;
	Blacklist MaximisedWindowBlacklist;
	OptionalTaskbarAppearance StartOpenedAppearance;
	OptionalTaskbarAppearance CortanaOpenedAppearance;
	OptionalTaskbarAppearance TimelineOpenedAppearance;

	// Peek
	PeekBehavior Peek;
	bool UseRegularAppearanceWhenPeeking;

	// Advanced
	bool HideTray;
	bool DisableSaving;
	bool VerboseLog;

	// Default-init with default settings
	inline Config() noexcept :
		RegularAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0 },
		MaximisedWindowAppearance { ACCENT_ENABLE_BLURBEHIND, 0xaa000000, true },
		MaximisedWindowBlacklist(),
		StartOpenedAppearance { ACCENT_NORMAL, 0, true },
		CortanaOpenedAppearance { ACCENT_NORMAL, 0, true },
		TimelineOpenedAppearance { ACCENT_NORMAL, 0, true },
		Peek(PeekBehavior::WindowMaximisedOnMainMonitor),
		UseRegularAppearanceWhenPeeking(true),
		HideTray(false),
		DisableSaving(false),
#ifdef _DEBUG
		VerboseLog(true)
#else
		VerboseLog(False)
#endif
	{ }

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(REGULAR_KEY.data(), REGULAR_KEY.length());
		writer.StartObject();
		RegularAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(MAXIMISED_KEY.data(), MAXIMISED_KEY.length());
		writer.StartObject();
		MaximisedWindowAppearance.Serialize(writer);
		writer.String(BLACKLIST_KEY.data(), BLACKLIST_KEY.length());
		writer.StartObject();
		MaximisedWindowBlacklist.Serialize(writer);
		writer.EndObject();
		writer.EndObject();

		writer.String(START_KEY.data(), START_KEY.length());
		writer.StartObject();
		StartOpenedAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(CORTANA_KEY.data(), CORTANA_KEY.length());
		writer.StartObject();
		CortanaOpenedAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(TIMELINE_KEY.data(), TIMELINE_KEY.length());
		writer.StartObject();
		TimelineOpenedAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(PEEK_KEY.data(), PEEK_KEY.length());
		const auto peek_str = Util::FindOrDefault(s_PeekMap, Peek);
		writer.String(peek_str.data(), peek_str.length());

		writer.String(REGULAR_ON_PEEK_KEY.data(), REGULAR_ON_PEEK_KEY.length());
		writer.Bool(UseRegularAppearanceWhenPeeking);

		writer.String(TRAY_KEY.data(), TRAY_KEY.length());
		writer.Bool(HideTray);

		writer.String(SAVING_KEY.data(), SAVING_KEY.length());
		writer.Bool(DisableSaving);

		writer.String(VERBOSE_KEY.data(), VERBOSE_KEY.length());
		writer.Bool(VerboseLog);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val);

private:
	static const std::unordered_map<PeekBehavior, std::wstring_view> s_PeekMap;

	static constexpr std::wstring_view REGULAR_KEY = L"regular_appearance";
	static constexpr std::wstring_view MAXIMISED_KEY = L"maximised_window_appearance";
	static constexpr std::wstring_view BLACKLIST_KEY = L"blacklist";
	static constexpr std::wstring_view START_KEY = L"start_opened_appearance";
	static constexpr std::wstring_view CORTANA_KEY = L"cortana_opened_appearance";
	static constexpr std::wstring_view TIMELINE_KEY = L"timeline_opened_appearance";
	static constexpr std::wstring_view PEEK_KEY = L"show_peek_button";
	static constexpr std::wstring_view REGULAR_ON_PEEK_KEY = L"regular_appearance_when_peeking";
	static constexpr std::wstring_view TRAY_KEY = L"hide_tray";
	static constexpr std::wstring_view SAVING_KEY = L"disable_saving";
	static constexpr std::wstring_view VERBOSE_KEY = L"verbose";
};

#pragma warning(pop)