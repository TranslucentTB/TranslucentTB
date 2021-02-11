#include "configmanager.hpp"
#include <cerrno>
#include <cstdio>
#include <rapidjson/encodedstream.h>
#include <rapidjson/error/error.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <share.h>
#include <Shlwapi.h>
#include <wil/resource.h>

#include "constants.hpp"
#include "../../ProgramLog/error/errno.hpp"
#include "../../ProgramLog/error/rapidjson.hpp"
#include "../../ProgramLog/error/std.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "../../ProgramLog/error/winrt.hpp"
#include "../../ProgramLog/log.hpp"
#include "win32.hpp"

std::filesystem::path ConfigManager::DetermineConfigPath(const std::optional<std::filesystem::path> &storageFolder)
{
	std::filesystem::path path;
	if (storageFolder)
	{
		path = *storageFolder / L"RoamingState";
	}
	else
	{
		const auto [loc, hr] = win32::GetExeLocation();
		HresultVerify(hr, spdlog::level::critical, L"Failed to determine executable location!");
		path = loc.parent_path();
	}

	path /= CONFIG_FILE;
	return path;
}

void ConfigManager::WatcherCallback(void *context, DWORD, std::wstring_view fileName)
{
	if (fileName.empty() || win32::IsSameFilename(fileName, CONFIG_FILE))
	{
		const auto that = static_cast<ConfigManager *>(context);

		that->Load();
		that->UpdateVerbosity();
		that->m_Callback(that->m_Context, that->m_Config);
	}
}

bool ConfigManager::TryOpenConfigAsJson() noexcept
{
	wil::unique_hkey key;
	const HRESULT hr = AssocQueryKey(ASSOCF_VERIFY | ASSOCF_INIT_IGNOREUNKNOWN, ASSOCKEY_SHELLEXECCLASS, L".json", L"open", key.put());
	if (SUCCEEDED(hr))
	{
		SHELLEXECUTEINFO info = {
			.cbSize = sizeof(info),
			.fMask = SEE_MASK_CLASSKEY | SEE_MASK_FLAG_NO_UI,
			.lpVerb = L"open",
			.lpFile = m_ConfigPath.c_str(),
			.nShow = SW_SHOW,
			.hkeyClass = key.get()
		};

		const bool success = ShellExecuteEx(&info);
		if (!success)
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to launch JSON file editor");
		}

		return success;
	}
	else
	{
		if (hr != HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION))
		{
			HresultHandle(hr, spdlog::level::warn, L"Failed to query for .json file association");
		}

		return false;
	}
}

void ConfigManager::SaveToFile(FILE *f) const
{
	static constexpr std::string_view COMMENT = "// See https://" UTF8_APP_NAME ".github.io/config for more information\n";
	static constexpr std::wstring_view SCHEMA = L"https://sylveon.dev/" APP_NAME L"/schema";

	char buffer[1024];
	rj::FileWriteStream filestream(f, buffer, std::size(buffer));

	using OutputStream = rj::EncodedOutputStream<rj::UTF8<>, rj::FileWriteStream>;
	OutputStream out(filestream, true);

	for (const char c : COMMENT)
	{
		out.Put(c);
	}

	rj::PrettyWriter<OutputStream, rj::UTF16LE<>> writer(out);

	writer.StartObject();
	rjh::Serialize(writer, SCHEMA, SCHEMA_KEY);
	m_Config.Serialize(writer);
	writer.EndObject();

	writer.Flush();
}

bool ConfigManager::LoadFromFile(FILE *f)
{
	static constexpr std::wstring_view DESERIALIZE_FAILED = L"Failed to deserialize JSON document";

	char buffer[1024];
	rj::FileReadStream filestream(f, buffer, std::size(buffer));

	rj::AutoUTFInputStream<uint32_t, rj::FileReadStream> in(filestream);

	rj::GenericDocument<rj::UTF16LE<>> doc;
	if (const rj::ParseResult result = doc.ParseStream<rj::kParseCommentsFlag, rj::AutoUTF<uint32_t>>(in))
	{
		// remove the schema key to avoid a false unknown key warning
		doc.RemoveMember(rjh::StringViewToValue(SCHEMA_KEY));

		try
		{
			// load the defaults before deserializing to not reuse previous settings
			// in case some keys are missing from the file
			m_Config = { };
			m_Config.Deserialize(doc, [](std::wstring_view unknownKey)
			{
				if (Error::ShouldLog(spdlog::level::info))
				{
					fmt::wmemory_buffer buf;
					fmt::format_to(buf, FMT_STRING(L"Unknown key found in JSON: {}"), unknownKey);
					MessagePrint(spdlog::level::info, buf);
				}
			});

			// everything went fine, we can return!
			return true;
		}
		HelperDeserializationErrorCatch(spdlog::level::err, DESERIALIZE_FAILED)
		StdSystemErrorCatch(spdlog::level::err, DESERIALIZE_FAILED);
	}
	else
	{
		ParseErrorCodeHandle(result.Code(), spdlog::level::err, DESERIALIZE_FAILED);
	}

	return false;
}

bool ConfigManager::Load()
{
	const wil::unique_file file(_wfsopen(m_ConfigPath.c_str(), L"rbS", _SH_DENYNO));
	const errno_t err = errno; // capture the error asap

	// skip empty files. this is to avoid a false error due to how some editors work.
	// notably, VS Code will create an empty file and then fill it.
	if (file &&
		!win32::IsFileEmpty(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file.get())))) &&
		LoadFromFile(file.get()))
	{
		return true;
	}
	else if (!file && err != ENOENT)
	{
		// if the file failed to open, but it exists, something went wrong
		ErrnoTHandle(err, spdlog::level::err, L"Failed to open configuration file");
	}

	// if anything failed, use defaults
	m_Config = { };

	// check if the file was successfully opened or it exists but failed to open
	return file || err != ENOENT;
}

ConfigManager::ConfigManager(const std::optional<std::filesystem::path> &storageFolder, bool &fileExists, callback_t callback, void *context) :
	m_ConfigPath(DetermineConfigPath(storageFolder)),
	m_Watcher(m_ConfigPath.parent_path(), false, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE, WatcherCallback, this),
	m_Callback(callback),
	m_Context(context)
{
	fileExists = Load();
	UpdateVerbosity();
}

void ConfigManager::UpdateVerbosity()
{
	if (const auto sink = Log::GetSink())
	{
		sink->set_level(m_Config.LogVerbosity);
	}
}

void ConfigManager::EditConfigFile()
{
	SaveConfig();

	if (!TryOpenConfigAsJson())
	{
		HresultVerify(win32::EditFile(m_ConfigPath), spdlog::level::err, L"Failed to open configuration file.");
	}
}

void ConfigManager::DeleteConfigFile()
{
	std::error_code errc;
	std::filesystem::remove(m_ConfigPath, errc);
	StdErrorCodeVerify(errc, spdlog::level::warn, L"Failed to delete config file");
}

void ConfigManager::SaveConfig() const
{
	if (m_Config.DisableSaving)
	{
		return;
	}

	std::filesystem::path tempFile = m_ConfigPath;
	tempFile.replace_extension(L".tmp");

	wil::unique_file file;
	const errno_t err = _wfopen_s(file.put(), tempFile.c_str(), L"wbS");
	if (err == 0)
	{
		SaveToFile(file.get());

		file.reset();
		if (!MoveFileEx(tempFile.c_str(), m_ConfigPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			LastErrorHandle(spdlog::level::err, L"Failed to move temporary configuration file");
		}
	}
	else
	{
		ErrnoTHandle(err, spdlog::level::err, L"Failed to save configuration!");
	}
}
