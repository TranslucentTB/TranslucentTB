#include "configmanager.hpp"
#include "winrt.hpp"
#include <winrt/Windows.Storage.h>

#include "constants.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "../ProgramLog/log.hpp"

std::filesystem::path ConfigManager::DetermineConfigPath(bool hasPackageIdentity)
{
	std::filesystem::path path;
	if (hasPackageIdentity)
	{
		try
		{
			path = std::wstring_view(winrt::Windows::Storage::ApplicationData::Current().RoamingFolder().Path());
		}
		HresultErrorCatch(spdlog::level::critical, L"Getting application folder paths failed!");
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

		const Config &cfg = that->m_Config.emplace(Config::Load(that->m_ConfigPath));
		that->UpdateVerbosity();
		that->m_Callback(that->m_Context, cfg);
	}
}

void ConfigManager::UpdateVerbosity()
{
	if (const auto sink = Log::GetSink())
	{
		sink->set_level(m_Config->LogVerbosity);
	}
}

Config &ConfigManager::GetConfig()
{
	if (!m_Config)
	{
		m_Config = Config::Load(m_ConfigPath);
		UpdateVerbosity();
		m_Watcher.emplace(m_ConfigPath.parent_path(), false, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE, WatcherCallback, this);
	}

	return *m_Config;
}
