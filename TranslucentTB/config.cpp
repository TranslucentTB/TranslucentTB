#include "config.hpp"

const std::unordered_map<ACCENT_STATE, std::wstring_view> TaskbarAppearance::s_AccentMap = {
	{ ACCENT_NORMAL,                     L"normal" },
	{ ACCENT_ENABLE_GRADIENT,            L"opaque" },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT, L"clear" },
	{ ACCENT_ENABLE_BLURBEHIND,          L"blur" },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   L"acrylic" }
};

void TaskbarAppearance::Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
{
	if (!val.IsObject())
	{
		return;
	}

	if (const auto accent = val.FindMember(L"accent"); accent != val.MemberEnd() && accent->value.IsString())
	{
		if (const auto iter = Util::FindValue(s_AccentMap, { accent->value.GetString(), accent->value.GetStringLength() }); iter != s_AccentMap.end())
		{
			Accent = iter->first;
		}
	}

	if (const auto color = val.FindMember(L"color"); color != val.MemberEnd() && color->value.IsString())
	{
		try
		{
			Color = (Color & 0xFF000000) + win32::SwapColorOrder(Util::ColorFromString({ color->value.GetString(), color->value.GetStringLength() }));
		}
		catch (const std::exception &)
		{
			// ignore
		}
	}
}

void OptionalTaskbarAppearance::Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
{
	if (!val.IsObject())
	{
		return;
	}

	if (const auto enabled = val.FindMember(L"enabled"); enabled != val.MemberEnd() && enabled->value.IsBool())
	{
		Enabled = enabled->value.GetBool();
	}

	TaskbarAppearance::Deserialize(val);
}

const std::unordered_map<PeekBehavior, std::wstring_view> Config::s_PeekMap = {
	{ PeekBehavior::AlwaysHide,                   L"never" },
	{ PeekBehavior::WindowMaximisedOnMainMonitor, L"when_maximised_window_on_main_monitor" },
	{ PeekBehavior::WindowMaximisedOnAnyMonitor,  L"when_maximised_window_on_any_monitor" },
	{ PeekBehavior::DesktopIsForegroundWindow,    L"when_desktop_is_foreground_window" },
	{ PeekBehavior::AlwaysShow,                   L"always" }

};

#undef GetObject

void Config::Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
{
	if (!val.IsObject())
	{
		return;
	}

	if (const auto regular = val.FindMember(L"regular_appearance"); regular != val.MemberEnd())
	{
		RegularAppearance.Deserialize(regular->value);
	}

	if (const auto maximised = val.FindMember(L"maximised_window_appearance"); maximised != val.MemberEnd())
	{
		const auto &value = maximised->value;
		MaximisedWindowAppearance.Deserialize(value);

		if (value.IsObject())
		{
			if (const auto blacklist = value.FindMember(L"blacklist"); blacklist != value.MemberEnd())
			{
				MaximisedWindowBlacklist.Deserialize(blacklist->value);
			}
		}
	}

	if (const auto start = val.FindMember(L"start_opened_appearance"); start != val.MemberEnd())
	{
		StartOpenedAppearance.Deserialize(start->value);
	}

	if (const auto cortana = val.FindMember(L"cortana_opened_appearance"); cortana != val.MemberEnd())
	{
		CortanaOpenedAppearance.Deserialize(cortana->value);
	}

	if (const auto timeline = val.FindMember(L"timeline_opened_appearance"); timeline != val.MemberEnd())
	{
		TimelineOpenedAppearance.Deserialize(timeline->value);
	}

	if (const auto regular_when_peeking = val.FindMember(L"regular_appearance_when_peeking"); regular_when_peeking != val.MemberEnd() && regular_when_peeking->value.IsBool())
	{
		UseRegularAppearanceWhenPeeking = regular_when_peeking->value.GetBool();
	}

	if (const auto peek = val.FindMember(L"show_peek_button"); peek != val.MemberEnd() && peek->value.IsString())
	{
		if (const auto iter = Util::FindValue(s_PeekMap, { peek->value.GetString(), peek->value.GetStringLength() }); iter != s_PeekMap.end())
		{
			Peek = iter->first;
		}
	}

	if (const auto no_tray = val.FindMember(L"hide_tray"); no_tray != val.MemberEnd() && no_tray->value.IsBool())
	{
		UseRegularAppearanceWhenPeeking = no_tray->value.GetBool();
	}

	if (const auto no_save = val.FindMember(L"disable_saving"); no_save != val.MemberEnd() && no_save->value.IsBool())
	{
		UseRegularAppearanceWhenPeeking = no_save->value.GetBool();
	}

	if (const auto verbose = val.FindMember(L"verbose"); verbose != val.MemberEnd() && verbose->value.IsBool())
	{
		UseRegularAppearanceWhenPeeking = verbose->value.GetBool();
	}
}
