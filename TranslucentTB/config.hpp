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

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	COLORREF     Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(L"accent");
		const auto accent_str = Util::FindOrDefault(s_AccentMap, Accent);
		writer.String(accent_str.data(), accent_str.length());

		writer.String(L"color");
		writer.String(Util::StringFromColor(win32::SwapColorOrder(Color)));

		writer.String(L"opacity");
		writer.Uint((Color & 0xFF000000) >> 24);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val);

private:
	static const std::unordered_map<ACCENT_STATE, std::wstring_view> s_AccentMap;
};

struct OptionalTaskbarAppearance : TaskbarAppearance {
	bool Enabled;

	template <typename Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(L"enabled");
		writer.Bool(Enabled);

		TaskbarAppearance::Serialize(writer);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val);
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
	inline Config() :
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
		writer.String(L"regular_appearance");
		writer.StartObject();
		RegularAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(L"maximised_window_appearance");
		writer.StartObject();
		MaximisedWindowAppearance.Serialize(writer);
		writer.String(L"blacklist");
		writer.StartObject();
		MaximisedWindowBlacklist.Serialize(writer);
		writer.EndObject();
		writer.EndObject();

		writer.String(L"start_opened_appearance");
		writer.StartObject();
		StartOpenedAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(L"cortana_opened_appearance");
		writer.StartObject();
		CortanaOpenedAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(L"timeline_opened_appearance");
		writer.StartObject();
		TimelineOpenedAppearance.Serialize(writer);
		writer.EndObject();

		writer.String(L"show_peek_button");
		const auto peek_str = Util::FindOrDefault(s_PeekMap, Peek);
		writer.String(peek_str.data(), peek_str.length());

		writer.String(L"regular_appearance_when_peeking");
		writer.Bool(UseRegularAppearanceWhenPeeking);

		writer.String(L"hide_tray");
		writer.Bool(HideTray);

		writer.String(L"disable_saving");
		writer.Bool(DisableSaving);

		writer.String(L"verbose");
		writer.Bool(VerboseLog);
	}

	void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val);

private:
	static const std::unordered_map<PeekBehavior, std::wstring_view> s_PeekMap;
};