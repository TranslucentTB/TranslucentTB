#include "config.hpp"

const std::unordered_map<ACCENT_STATE, std::wstring_view> TaskbarAppearance::s_AccentMap = {
	{ ACCENT_NORMAL,                     L"normal"  },
	{ ACCENT_ENABLE_GRADIENT,            L"opaque"  },
	{ ACCENT_ENABLE_TRANSPARENTGRADIENT, L"clear"   },
	{ ACCENT_ENABLE_BLURBEHIND,          L"blur"    },
	{ ACCENT_ENABLE_ACRYLICBLURBEHIND,   L"acrylic" }
};

void TaskbarAppearance::Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
{
	if (!val.IsObject())
	{
		return;
	}

	if (const auto accent = val.FindMember(ACCENT_KEY.data()); accent != val.MemberEnd() && accent->value.IsString())
	{
		if (const auto iter = Util::FindValue(s_AccentMap, { accent->value.GetString(), accent->value.GetStringLength() }); iter != s_AccentMap.end())
		{
			Accent = iter->first;
		}
	}

	if (const auto color = val.FindMember(COLOR_KEY.data()); color != val.MemberEnd() && color->value.IsString())
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

	if (const auto opacity = val.FindMember(OPACITY_KEY.data()); opacity != val.MemberEnd() && opacity->value.IsInt())
	{
		Color = (std::clamp(opacity->value.GetInt(), 0, 255) << 24) + (Color & 0xFFFFFF);
	}
}

void OptionalTaskbarAppearance::Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
{
	if (!val.IsObject())
	{
		return;
	}

	if (const auto enabled = val.FindMember(ENABLED_KEY.data()); enabled != val.MemberEnd() && enabled->value.IsBool())
	{
		Enabled = enabled->value.GetBool();
	}

	TaskbarAppearance::Deserialize(val);
}

const std::unordered_map<PeekBehavior, std::wstring_view> Config::s_PeekMap = {
	{ PeekBehavior::AlwaysHide,                   L"never"                                 },
	{ PeekBehavior::WindowMaximisedOnMainMonitor, L"when_maximised_window_on_main_monitor" },
	{ PeekBehavior::WindowMaximisedOnAnyMonitor,  L"when_maximised_window_on_any_monitor"  },
	{ PeekBehavior::DesktopIsForegroundWindow,    L"when_desktop_is_foreground_window"     },
	{ PeekBehavior::AlwaysShow,                   L"always"                                }

};

#undef GetObject

void Config::Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &val)
{
	if (!val.IsObject())
	{
		return;
	}

	if (const auto regular = val.FindMember(REGULAR_KEY.data()); regular != val.MemberEnd())
	{
		RegularAppearance.Deserialize(regular->value);
	}

	if (const auto maximised = val.FindMember(MAXIMISED_KEY.data()); maximised != val.MemberEnd())
	{
		const auto &value = maximised->value;
		MaximisedWindowAppearance.Deserialize(value);

		if (value.IsObject())
		{
			if (const auto blacklist = value.FindMember(BLACKLIST_KEY.data()); blacklist != value.MemberEnd())
			{
				MaximisedWindowBlacklist.Deserialize(blacklist->value);
			}
		}
	}

	if (const auto start = val.FindMember(START_KEY.data()); start != val.MemberEnd())
	{
		StartOpenedAppearance.Deserialize(start->value);
	}

	if (const auto cortana = val.FindMember(CORTANA_KEY.data()); cortana != val.MemberEnd())
	{
		CortanaOpenedAppearance.Deserialize(cortana->value);
	}

	if (const auto timeline = val.FindMember(TIMELINE_KEY.data()); timeline != val.MemberEnd())
	{
		TimelineOpenedAppearance.Deserialize(timeline->value);
	}


	if (const auto peek = val.FindMember(PEEK_KEY.data()); peek != val.MemberEnd() && peek->value.IsString())
	{
		if (const auto iter = Util::FindValue(s_PeekMap, { peek->value.GetString(), peek->value.GetStringLength() }); iter != s_PeekMap.end())
		{
			Peek = iter->first;
		}
	}

	if (const auto regular_when_peeking = val.FindMember(REGULAR_ON_PEEK_KEY.data()); regular_when_peeking != val.MemberEnd() && regular_when_peeking->value.IsBool())
	{
		UseRegularAppearanceWhenPeeking = regular_when_peeking->value.GetBool();
	}

	if (const auto no_tray = val.FindMember(TRAY_KEY.data()); no_tray != val.MemberEnd() && no_tray->value.IsBool())
	{
		HideTray = no_tray->value.GetBool();
	}

	if (const auto no_save = val.FindMember(SAVING_KEY.data()); no_save != val.MemberEnd() && no_save->value.IsBool())
	{
		DisableSaving = no_save->value.GetBool();
	}

	if (const auto verbose = val.FindMember(VERBOSE_KEY.data()); verbose != val.MemberEnd() && verbose->value.IsBool())
	{
		VerboseLog = verbose->value.GetBool();
	}
}
