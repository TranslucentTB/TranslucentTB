#pragma once
#include <filesystem>
#include <optional>
#include <string_view>
#include <type_traits>

#include "config/config.hpp"
#include "folderwatcher.hpp"

// TODO: move Config::Load and Config::Save in ConfigManager?
class ConfigManager {
	using callback_t = std::add_pointer_t<void(void *, const Config &)>;

	static std::filesystem::path DetermineConfigPath(bool hasPackageIdentity);
	static void WatcherCallback(void *context, DWORD, std::wstring_view fileName);

	std::filesystem::path m_ConfigPath;
	std::optional<Config> m_Config;
	std::optional<FolderWatcher> m_Watcher;

	callback_t m_Callback;
	void *m_Context;

public:
	inline ConfigManager(bool hasPackageIdentity, callback_t callback, void *context) : m_ConfigPath(DetermineConfigPath(hasPackageIdentity)), m_Callback(callback), m_Context(context) { }

	void UpdateVerbosity();
	bool LoadConfig();
	Config &GetConfig();

	constexpr const std::filesystem::path &GetConfigPath() const noexcept
	{
		return m_ConfigPath;
	}

	inline void SaveConfig()
	{
		GetConfig().Save(m_ConfigPath);
	}
};
