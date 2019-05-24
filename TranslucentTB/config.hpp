#pragma once
#include <string>
#include <string_view>

#include "undoc/swca.hpp"

enum class PeekBehavior {
	AlwaysHide,                   // Always hide the button
	WindowMaximisedOnMainMonitor, // Show when a window is maximised on the main monitor
	WindowMaximisedOnAnyMonitor,  // Show when a window is maximised on any monitor
	DesktopIsForegroundWindow,    // Show when the desktop is the foreground window
	AlwaysShow                    // Always show the button
};

struct TaskbarAppearance {
	ACCENT_STATE Accent;
	COLORREF     Color;

	template<class Writer>
	inline void Serialize(Writer &writer) const
	{
		writer.String(L"accent");
		const auto accent_str = AccentToString();
		writer.String(accent_str.data(), accent_str.length());

		writer.String(L"color");
		writer.String(ColorToString());

		writer.String(L"opacity");
		writer.Uint((Color & 0xFF000000) >> 24);
	}

private:
	std::wstring_view AccentToString() const;
	std::wstring ColorToString() const;
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
};

class Config {
public:
	// Appearances
	TaskbarAppearance RegularAppearance;
	OptionalTaskbarAppearance MaximisedWindowAppearance;
	OptionalTaskbarAppearance StartOpenedAppearance;
	OptionalTaskbarAppearance CortanaOpenedAppearance;
	OptionalTaskbarAppearance TimelineOpenedAppearance;

	// Peek
	bool UseRegularAppearanceWhenPeeking;
	PeekBehavior Peek;

	// Advanced
	bool HideTray;
	bool DisableSaving;
	bool VerboseLog;

	// Default-init with default settings
	Config() :
		RegularAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0 },
		MaximisedWindowAppearance { ACCENT_ENABLE_BLURBEHIND, 0xaa000000, true },
		StartOpenedAppearance { ACCENT_NORMAL, 0, true },
		CortanaOpenedAppearance { ACCENT_NORMAL, 0, true },
		TimelineOpenedAppearance { ACCENT_NORMAL, 0, true },
		UseRegularAppearanceWhenPeeking(true),
		Peek(PeekBehavior::WindowMaximisedOnMainMonitor),
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

		writer.String(L"regular_appearance_when_peeking");
		writer.Bool(UseRegularAppearanceWhenPeeking);

		writer.String(L"show_peek_button");
		const auto peek_str = PeekToString();
		writer.String(peek_str.data(), peek_str.length());

		writer.String(L"hide_tray");
		writer.Bool(HideTray);

		writer.String(L"disable_saving");
		writer.Bool(DisableSaving);

		writer.String(L"verbose");
		writer.Bool(VerboseLog);
	}

private:
	std::wstring_view PeekToString() const;
};