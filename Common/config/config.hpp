#pragma once
#include <array>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <spdlog/common.h>
#include <string_view>

#include "../util/null_terminated_string_view.hpp"
#include "optionaltaskbarappearance.hpp"
#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"
#include "windowfilter.hpp"


#ifdef _TRANSLUCENTTB_EXE
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <rapidjson/encodedstream.h>
#include <rapidjson/error/error.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <share.h>
#include <wil/resource.h>

#include "../appinfo.hpp"
#include "../../ProgramLog/error/rapidjson.hpp"
#include "../../ProgramLog/error/std.hpp"
#endif

class Config {
public:
	// Appearances
	TaskbarAppearance DesktopAppearance;
	OptionalTaskbarAppearance VisibleWindowAppearance;
	OptionalTaskbarAppearance MaximisedWindowAppearance;
	OptionalTaskbarAppearance StartOpenedAppearance;
	OptionalTaskbarAppearance CortanaOpenedAppearance;
	OptionalTaskbarAppearance TimelineOpenedAppearance;

	// Advanced
	WindowFilter Whitelist;
	WindowFilter Blacklist;
	bool HideTray;
	bool DisableSaving;
	spdlog::level::level_enum LogVerbosity;

	// Default-init with default settings
	inline Config() noexcept :
		DesktopAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0 },
		VisibleWindowAppearance { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0, false },
		MaximisedWindowAppearance { ACCENT_ENABLE_BLURBEHIND, 0xAA000000, true },
		StartOpenedAppearance { ACCENT_NORMAL, 0, true },
		CortanaOpenedAppearance { ACCENT_NORMAL, 0, true },
		TimelineOpenedAppearance { ACCENT_NORMAL, 0, true },
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
		RapidJSONHelper::Serialize(writer, Whitelist, WHITELIST_KEY);
		RapidJSONHelper::Serialize(writer, Blacklist, BLACKLIST_KEY);
		RapidJSONHelper::Serialize(writer, HideTray, TRAY_KEY);
		RapidJSONHelper::Serialize(writer, DisableSaving, SAVING_KEY);
		RapidJSONHelper::Serialize(writer, LogVerbosity, LOG_KEY, LOG_MAP);
	}

	inline void Deserialize(const rapidjson::GenericValue<rapidjson::UTF16LE<>> &obj)
	{
		RapidJSONHelper::Deserialize(obj, DesktopAppearance, DESKTOP_KEY);
		RapidJSONHelper::Deserialize(obj, VisibleWindowAppearance, VISIBLE_KEY);
		RapidJSONHelper::Deserialize(obj, MaximisedWindowAppearance, MAXIMISED_KEY);
		RapidJSONHelper::Deserialize(obj, StartOpenedAppearance, START_KEY);
		RapidJSONHelper::Deserialize(obj, CortanaOpenedAppearance, CORTANA_KEY);
		RapidJSONHelper::Deserialize(obj, TimelineOpenedAppearance, TIMELINE_KEY);
		RapidJSONHelper::Deserialize(obj, Whitelist, WHITELIST_KEY);
		RapidJSONHelper::Deserialize(obj, Blacklist, BLACKLIST_KEY);
		RapidJSONHelper::Deserialize(obj, HideTray, TRAY_KEY);
		RapidJSONHelper::Deserialize(obj, DisableSaving, SAVING_KEY);
		RapidJSONHelper::Deserialize(obj, LogVerbosity, LOG_KEY, LOG_MAP);
	}

#ifdef _TRANSLUCENTTB_EXE
	inline void Save(const std::filesystem::path &file, bool ignoreDisabledSaving = false)
	{
		if (ignoreDisabledSaving || !DisableSaving)
		{
			wil::unique_file pfile;
			const errno_t err = _wfopen_s(pfile.put(), file.c_str(), L"wbS");
			if (err == 0)
			{
				using namespace rapidjson;

				char buffer[256];
				FileWriteStream filestream(pfile.get(), buffer, std::size(buffer));

				using OutputStream = EncodedOutputStream<UTF16LE<>, FileWriteStream>;
				OutputStream out(filestream, true);

				static constexpr std::wstring_view comment = L"// See https://" APP_NAME L".github.io/config for more info\n";
				for (const wchar_t c : comment)
				{
					out.Put(c);
				}

				PrettyWriter<OutputStream, UTF16LE<>, UTF16LE<>> writer(out);

				writer.StartObject();
				Serialize(writer);
				writer.EndObject();

				writer.Flush();
			}
			else
			{
				ErrnoTHandle(err, spdlog::level::err, L"Failed to save configuration!");
			}
		}
	}

	inline static Config Load(const std::filesystem::path &file)
	{
		// This check is so that if the file gets deleted for whatever reason while the app is running, default configuration gets restored immediatly.
		if (std::filesystem::is_regular_file(file))
		{
			wil::unique_file pfile(_wfsopen(file.c_str(), L"rbS", _SH_DENYNO));
			if (pfile)
			{
				using namespace rapidjson;

				char buffer[256];
				FileReadStream filestream(pfile.get(), buffer, std::size(buffer));

				AutoUTFInputStream<uint32_t, FileReadStream> in(filestream);

				GenericDocument<UTF16LE<>> doc;
				if (ParseResult result = doc.ParseStream<kParseCommentsFlag, AutoUTF<uint32_t>>(in))
				{
					static constexpr std::wstring_view ERR_MSG = L"Failed to deserialize JSON document";
					try
					{
						RapidJSONHelper::EnsureType(rapidjson::Type::kObjectType, doc.GetType(), L"root node");

						Config cfg;
						cfg.Deserialize(doc);
						return cfg;
					}
					HelperDeserializationErrorCatch(spdlog::level::err, ERR_MSG)
					StdSystemErrorCatch(spdlog::level::err, ERR_MSG);
				}
				else
				{
					ParseErrorCodeHandle(result.Code(), spdlog::level::err, L"Failed to parse configuration!");
				}
			}
			else
			{
				ErrnoHandle(spdlog::level::err, L"Failed to load configuration!");
			}
		}

		return { };
	}
#endif

private:
	static constexpr std::array<std::wstring_view, spdlog::level::off + 1> LOG_MAP = {
		L"trace",
		L"debug",
		L"info",
		L"warn",
		L"err",
		L"critical",
		L"off"
	};

	static constexpr Util::null_terminated_wstring_view DESKTOP_KEY = L"desktop_appearance";
	static constexpr Util::null_terminated_wstring_view VISIBLE_KEY = L"visible_window_appearance";
	static constexpr Util::null_terminated_wstring_view MAXIMISED_KEY = L"maximised_window_appearance";
	static constexpr Util::null_terminated_wstring_view START_KEY = L"start_opened_appearance";
	static constexpr Util::null_terminated_wstring_view CORTANA_KEY = L"cortana_opened_appearance";
	static constexpr Util::null_terminated_wstring_view TIMELINE_KEY = L"timeline_opened_appearance";
	static constexpr Util::null_terminated_wstring_view WHITELIST_KEY = L"whitelist";
	static constexpr Util::null_terminated_wstring_view BLACKLIST_KEY = L"blacklist";
	static constexpr Util::null_terminated_wstring_view TRAY_KEY = L"hide_tray";
	static constexpr Util::null_terminated_wstring_view SAVING_KEY = L"disable_saving";
	static constexpr Util::null_terminated_wstring_view LOG_KEY = L"verbosity";
};
