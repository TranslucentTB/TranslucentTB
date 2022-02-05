#pragma once
#include "arch.h"
#include <filesystem>
#include <optional>
#include <string_view>
#include <synchapi.h>
#include <type_traits>
#include <wil/resource.h>

#include "config/config.hpp"
#include "../folderwatcher.hpp"

class ConfigManager {
	// we use settings.json because it makes VS Code automatically recognize
	// the file as JSON with comments
	static constexpr std::wstring_view CONFIG_FILE = L"settings.json";
	static constexpr std::wstring_view SCHEMA_KEY = L"$schema";

	using callback_t = std::add_pointer_t<void(void *)>;

	static std::filesystem::path DetermineConfigPath(const std::optional<std::filesystem::path> &storageFolder);
	static void WatcherCallback(void *context, DWORD, std::wstring_view fileName);
	static void APIENTRY TimerCallback(void *context, DWORD timerLow, DWORD timerHigh);

	std::filesystem::path m_ConfigPath;
	Config m_Config;
	FolderWatcher m_Watcher;

	wil::unique_handle m_ReloadTimer;

	callback_t m_Callback;
	void *m_Context;

	bool TryOpenConfigAsJson() noexcept;
	void SaveToFile(FILE *f) const;
	void LoadFromFile(FILE *f);
	bool Load();
	void Reload();
	bool ScheduleReload();

public:
	ConfigManager(const std::optional<std::filesystem::path> &storageFolder, bool &fileExists, callback_t callback, void *context);
	~ConfigManager();

	void UpdateVerbosity();
	void EditConfigFile();
	void DeleteConfigFile();
	void SaveConfig() const;

	constexpr Config &GetConfig() noexcept
	{
		return m_Config;
	}
};
