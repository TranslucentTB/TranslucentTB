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

		that->LoadConfig();
		that->m_Callback(that->m_Context, *that->m_Config);
	}
}

void ConfigManager::UpdateVerbosity()
{
	if (const auto sink = Log::GetSink())
	{
		sink->set_level(GetConfig().LogVerbosity);
	}
}

bool ConfigManager::LoadConfig()
{
	bool fileExists = false;
	bool hasPreviousConfig = m_Config.has_value();

	m_Config = Config::Load(m_ConfigPath, fileExists);
	UpdateVerbosity();

	if (!hasPreviousConfig)
	{
		m_Watcher.emplace(m_ConfigPath.parent_path(), false, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE, WatcherCallback, this);
	}

	return fileExists;
}

Config &ConfigManager::GetConfig()
{
	if (!m_Config)
	{
		LoadConfig();
	}

	return *m_Config;
}
