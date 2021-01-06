#pragma once
#include <array>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <spdlog/common.h>
#include <string_view>

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
#include "../../ProgramLog/error/errno.hpp"
#include "../../ProgramLog/error/rapidjson.hpp"
#include "../../ProgramLog/error/std.hpp"
#endif

class Config {
public:
	// Appearances
	TaskbarAppearance DesktopAppearance = { ACCENT_ENABLE_TRANSPARENTGRADIENT, { } };
	OptionalTaskbarAppearance VisibleWindowAppearance = { ACCENT_ENABLE_BLURBEHIND, { }, false };
	OptionalTaskbarAppearance MaximisedWindowAppearance = { ACCENT_ENABLE_BLURBEHIND, { }, true };
	OptionalTaskbarAppearance StartOpenedAppearance = { ACCENT_NORMAL, { }, true };
	OptionalTaskbarAppearance CortanaOpenedAppearance = { ACCENT_NORMAL, { }, true };
	OptionalTaskbarAppearance TimelineOpenedAppearance = { ACCENT_NORMAL, { }, true };

	// Advanced
	WindowFilter Whitelist;
	WindowFilter IgnoredWindows;
	bool HideTray = false;
	bool DisableSaving = false;
	spdlog::level::level_enum LogVerbosity =
#ifdef _DEBUG
		spdlog::level::debug;
#else
		spdlog::level::warn;
#endif

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
		RapidJSONHelper::Serialize(writer, IgnoredWindows, IGNORED_WINDOWS_KEY);
		RapidJSONHelper::Serialize(writer, HideTray, TRAY_KEY);
		RapidJSONHelper::Serialize(writer, DisableSaving, SAVING_KEY);
		RapidJSONHelper::Serialize(writer, LogVerbosity, LOG_KEY, LOG_MAP);
	}

	inline void Deserialize(const RapidJSONHelper::value_t &obj)
	{
		RapidJSONHelper::Deserialize(obj, DesktopAppearance, DESKTOP_KEY);
		RapidJSONHelper::Deserialize(obj, VisibleWindowAppearance, VISIBLE_KEY);
		RapidJSONHelper::Deserialize(obj, MaximisedWindowAppearance, MAXIMISED_KEY);
		RapidJSONHelper::Deserialize(obj, StartOpenedAppearance, START_KEY);
		RapidJSONHelper::Deserialize(obj, CortanaOpenedAppearance, CORTANA_KEY);
		RapidJSONHelper::Deserialize(obj, TimelineOpenedAppearance, TIMELINE_KEY);
		RapidJSONHelper::Deserialize(obj, Whitelist, WHITELIST_KEY);
		RapidJSONHelper::Deserialize(obj, IgnoredWindows, IGNORED_WINDOWS_KEY);
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

				char buffer[1024];
				FileWriteStream filestream(pfile.get(), buffer, std::size(buffer));

				using OutputStream = EncodedOutputStream<UTF8<>, FileWriteStream>;
				OutputStream out(filestream, true);

				static constexpr std::string_view comment = "// See https://" UTF8_APP_NAME ".github.io/config for more information\n";
				for (const char c : comment)
				{
					out.Put(c);
				}

				PrettyWriter<OutputStream, UTF16LE<>> writer(out);

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

	inline static Config Load(const std::filesystem::path &file, bool &fileExists)
	{
		fileExists = true;
		wil::unique_file pfile(_wfsopen(file.c_str(), L"rbS", _SH_DENYNO));
		if (pfile)
		{
			using namespace rapidjson;

			char buffer[1024];
			FileReadStream filestream(pfile.get(), buffer, std::size(buffer));

			AutoUTFInputStream<uint32_t, FileReadStream> in(filestream);

			GenericDocument<UTF16LE<>> doc;
			if (const ParseResult result = doc.ParseStream<kParseCommentsFlag, AutoUTF<uint32_t>>(in))
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
			// It's not an error for the config file to not exist.
			if (const errno_t err = errno; err == ENOENT)
			{
				fileExists = false;
			}
			else
			{
				ErrnoTHandle(err, spdlog::level::err, L"Failed to load configuration!");
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

	static constexpr std::wstring_view DESKTOP_KEY = L"desktop_appearance";
	static constexpr std::wstring_view VISIBLE_KEY = L"visible_window_appearance";
	static constexpr std::wstring_view MAXIMISED_KEY = L"maximised_window_appearance";
	static constexpr std::wstring_view START_KEY = L"start_opened_appearance";
	static constexpr std::wstring_view CORTANA_KEY = L"cortana_opened_appearance";
	static constexpr std::wstring_view TIMELINE_KEY = L"timeline_opened_appearance";
	static constexpr std::wstring_view WHITELIST_KEY = L"whitelist";
	static constexpr std::wstring_view IGNORED_WINDOWS_KEY = L"ignored_windows";
	static constexpr std::wstring_view TRAY_KEY = L"hide_tray";
	static constexpr std::wstring_view SAVING_KEY = L"disable_saving";
	static constexpr std::wstring_view LOG_KEY = L"verbosity";
};
