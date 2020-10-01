#pragma once
#include <filesystem>
#include <optional>
#include <string_view>
#include <type_traits>

#include "config/config.hpp"
#include "../folderwatcher.hpp"

// TODO: move Config::Load and Config::Save in ConfigManager?
class ConfigManager {
	using callback_t = std::add_pointer_t<void(void *, const Config &)>;

	static std::filesystem::path DetermineConfigPath(bool hasPackageIdentity);
	static void WatcherCallback(void *context, DWORD, std::wstring_view fileName);

	std::filesystem::path m_ConfigPath;
	Config m_Config;
	FolderWatcher m_Watcher;

	callback_t m_Callback;
	void *m_Context;

public:
	ConfigManager(bool hasPackageIdentity, bool &fileExists, callback_t callback, void *context);

	void UpdateVerbosity();

	constexpr Config &GetConfig() noexcept
	{
		return m_Config;
	}

	constexpr const std::filesystem::path &GetConfigPath() const noexcept
	{
		return m_ConfigPath;
	}

	inline void SaveConfig()
	{
		GetConfig().Save(m_ConfigPath);
	}
};
