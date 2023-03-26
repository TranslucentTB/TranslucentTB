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
#include "winrt.hpp"
#include <winrt/Windows.ApplicationModel.Resources.Core.h>

#include "constants.hpp"
#include "../../ProgramLog/error/errno.hpp"
#include "../../ProgramLog/error/rapidjson.hpp"
#include "../../ProgramLog/error/std.hpp"
#include "../../ProgramLog/error/win32.hpp"
#include "../../ProgramLog/error/winrt.hpp"
#include "../../ProgramLog/log.hpp"
#include "config/rapidjsonhelper.hpp"
#include "win32.hpp"
#include "../localization.hpp"
#include "../resources/ids.h"

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

		if (!that->ScheduleReload())
		{
			that->Reload();
		}
	}
}

void ConfigManager::TimerCallback(void *context, DWORD, DWORD)
{
	static_cast<ConfigManager *>(context)->Reload();
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
	writer.SetIndent(' ', 2);

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
				if (Error::ShouldLog<spdlog::level::info>())
				{
					MessagePrint(spdlog::level::info, std::format(L"Unknown key found in JSON: {}", unknownKey));
				}
			});

			// everything went fine, we can return!
			return true;
		}
		HelperDeserializationErrorCatch(spdlog::level::err, DESERIALIZE_FAILED)
		StdSystemErrorCatch(spdlog::level::err, DESERIALIZE_FAILED);
	}
	else if (result.Code() != rj::kParseErrorDocumentEmpty)
	{
		ParseErrorCodeHandle(result.Code(), spdlog::level::err, DESERIALIZE_FAILED);
	}

	// parsing failed, use defaults
	m_Config = { };
	return false;
}

bool ConfigManager::Load(bool firstLoad)
{
	if (const wil::unique_file file { _wfsopen(m_ConfigPath.c_str(), L"rbS", _SH_DENYNO) })
	{
		if (LoadFromFile(file.get()))
		{
			if (firstLoad)
			{
				m_StartupLanguage = m_Config.Language;

				if (!m_Config.Language.empty())
				{
					std::wstring langOverride = m_Config.Language;
					// make double null terminated
					langOverride.push_back(L'\0');
					if (!SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, langOverride.c_str(), nullptr))
					{
						LastErrorHandle(spdlog::level::err, L"Failed to set process UI language. Is the language set in the configuration file a BCP-47 language name?");

						// don't try setting XAML language, it'll probably fail too
						return true;
					}

					// SetProcessPreferredUILanguages does not affect the lookup behavior of resource functions like FindResourceEx
					// only SetThreadPreferredUILanguages does.
					// WHY WINDOWS
					// WHAT IS THE POINT OF SETPROCESSPREFERREDUILANGUAGES THEN
					if (!SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, langOverride.c_str(), nullptr))
					{
						LastErrorHandle(spdlog::level::err, L"Failed to set thread UI language. Is the language set in the configuration file a BCP-47 language name?");

						// remove the existing override to not fail in a partially localized to previous value state
						SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, nullptr, nullptr);

						// don't try setting XAML language, it'll probably fail too
						return true;
					}

					try
					{
						winrt::Windows::ApplicationModel::Resources::Core::ResourceContext::SetGlobalQualifierValue(L"Language", m_Config.Language);
					}
					catch (const winrt::hresult_error &err)
					{
						HresultErrorHandle(err, spdlog::level::err, L"Failed to set resource language override. Is the language set in the configuration file a BCP-47 language name?");

						// remove the existing overrides to not fail in a partially localized to previous value state
						SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, nullptr, nullptr);
						SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, nullptr, nullptr);
					}
				}
			}
			else if (m_StartupLanguage != m_Config.Language && !std::exchange(m_ShownChangeWarning, true))
			{
				std::thread([msg = Localization::LoadLocalizedResourceString(IDS_LANGUAGE_CHANGED, wil::GetModuleInstanceHandle())]() noexcept
				{
					MessageBoxEx(Window::NullWindow, msg.c_str(), APP_NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
				}).detach();
			}
		}

		// note: this demarks if the file exists, even if parsing failed.
		return true;
	}
	else
	{
		const errno_t err = errno;
		const bool fileExists = err != ENOENT;
		if (fileExists)
		{
			// if the file failed to open, but it exists, something went wrong
			ErrnoTHandle(err, spdlog::level::err, L"Failed to open configuration file");
		}

		// opening file failed, use defaults
		m_Config = { };
		return fileExists;
	}
}

void ConfigManager::Reload()
{
	Load();
	UpdateVerbosity();
	m_Callback(m_Context);
}

bool ConfigManager::ScheduleReload()
{
	if (m_ReloadTimer)
	{
		LARGE_INTEGER waitTime{};
		// 200 ms, relative to current time. arbitrarily chosen to get a balance
		// between feeling snappy and not triggering errors due to the various weird
		// ways editors save files.
		waitTime.QuadPart = -2000000;
		if (SetWaitableTimer(m_ReloadTimer.get(), &waitTime, 0, TimerCallback, this, false))
		{
			return true;
		}
		else
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to set waitable timer");
		}
	}

	return false;
}

ConfigManager::ConfigManager(const std::optional<std::filesystem::path> &storageFolder, bool &fileExists, callback_t callback, void *context) :
	m_ConfigPath(DetermineConfigPath(storageFolder)),
	m_Watcher(m_ConfigPath.parent_path(), false, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE, WatcherCallback, this),
	m_ReloadTimer(CreateWaitableTimer(nullptr, true, nullptr)),
	m_ShownChangeWarning(false),
	m_Callback(callback),
	m_Context(context)
{
	if (!m_ReloadTimer)
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to create waitable timer");
	}

	fileExists = Load(true);
	UpdateVerbosity();
}

ConfigManager::~ConfigManager()
{
	if (m_ReloadTimer)
	{
		if (!CancelWaitableTimer(m_ReloadTimer.get()))
		{
			LastErrorHandle(spdlog::level::info, L"Failed to cancel reload timer");
		}
	}
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
		if (!ReplaceFile(m_ConfigPath.c_str(), tempFile.c_str(), nullptr, REPLACEFILE_WRITE_THROUGH | REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, nullptr, nullptr))
		{
			// If the target file doesn't exist (e.g. brand new installation of TranslucentTB), ReplaceFile fails.
			if (const auto lastErr = GetLastError(); lastErr == ERROR_FILE_NOT_FOUND)
			{
				if (!MoveFileEx(tempFile.c_str(), m_ConfigPath.c_str(), MOVEFILE_WRITE_THROUGH))
				{
					LastErrorHandle(spdlog::level::err, L"Failed to move temporary configuration file");
				}
			}
			else
			{
				HresultHandle(HRESULT_FROM_WIN32(lastErr), spdlog::level::err, L"Failed to replace configuration file");
			}
		}
	}
	else
	{
		ErrnoTHandle(err, spdlog::level::err, L"Failed to save configuration!");
	}
}
