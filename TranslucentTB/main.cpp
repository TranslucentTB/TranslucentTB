#include "arch.h"
#include <filesystem>
#include <sal.h>
#include <wil/resource.h>
#include <Unknwn.h>
#include <winrt/base.h>
#include <winrt/Windows.Storage.h>

#include "constants.hpp"
#include "../ProgramLog/error/win32.hpp"
#include "mainappwindow.hpp"
#include "uwp.hpp"
#include "taskdialogs/welcomedialog.hpp"
#include "win32.hpp"
#include "windows/windowclass.hpp"

/*
template<class T>
void BindByMap(ContextMenu &menu, const std::unordered_map<T, unsigned int> &map, std::type_identity_t<std::function<T()>> getter, std::type_identity_t<std::function<void(T)>> setter)
{
	for (const auto &[new_value, id] : map)
	{
		menu.RegisterCallback(id, [&new_value, setter]
		{
			setter(new_value);
		});
	}

	const auto [min_p, max_p] = std::minmax_element(map.begin(), map.end(), Util::map_value_compare<T, unsigned int>());

	menu.RegisterCustomRefresh([min = min_p->second, max = max_p->second, get = std::move(getter), &map](ContextMenu::Updater updater)
	{
		updater.CheckRadio(min, max, map.at(get()));
	});
}

template<class T>
void BindByMap(ContextMenu &menu, const std::unordered_map<T, unsigned int> &map, T &value)
{
	BindByMap(
		menu,
		map,
		[&value] { return value; },
		[&value](T new_value) { value = new_value; }
	);
}

void BindBool(ContextMenu &menu, unsigned int item, bool &value)
{
	menu.RegisterCallback(item, [&value]
	{
		value = !value;
	});

	menu.RegisterCustomRefresh([item, &value](ContextMenu::Updater updater)
	{
		updater.CheckItem(item, value);
	});
}

void BindBoolToEnabled(ContextMenu &menu, unsigned int item, bool &value)
{
	menu.RegisterCustomRefresh([item, &value](ContextMenu::Updater updater)
	{
		updater.EnableItem(item, value);
	});
}

void BindAppearance(ContextMenu &menu, TaskbarAppearance &appearance, unsigned int colorId, const std::unordered_map<ACCENT_STATE, uint32_t> &map)
{
	BindColor(menu, colorId, appearance.Color);
	BindByMap(menu, map, appearance.Accent);
}

void BindAppearance(ContextMenu &menu, OptionalTaskbarAppearance &appearance, unsigned int enableId, unsigned int colorId, const std::unordered_map<ACCENT_STATE, uint32_t> &map)
{
	BindBool(menu, enableId, appearance.Enabled);
	BindAppearance(menu, appearance, colorId, map);
	for (const auto &[_, id] : map)
	{
		BindBoolToEnabled(menu, id, appearance.Enabled);
	}
}

void EnableAppearanceColor(ContextMenu::Updater updater, unsigned int id, const OptionalTaskbarAppearance &appearance)
{
	updater.EnableItem(id, appearance.Enabled && appearance.Accent != ACCENT_NORMAL);
}

winrt::fire_and_forget RefreshMenu(const Config &cfg, ContextMenu::Updater updater)
{
	// Fire off the task and do what we can do before blocking
	updater.EnableItem(ID_AUTOSTART, false);
	updater.CheckItem(ID_AUTOSTART, false);

	using namespace winrt::Windows;
	Foundation::IAsyncOperation<ApplicationModel::StartupTaskState> task;
	if (UWP::HasPackageIdentity())
	{
		task = Autostart::GetStartupState();
		updater.SetText(ID_AUTOSTART, IDS_AUTOSTART_QUERYING);
	}

	const auto status = Log::GetStatus();
	updater.EnableItem(ID_OPENLOG, status == Log::Ok);
	updater.SetText(ID_OPENLOG, status == Log::Ok
		? IDS_OPENLOG_NORMAL
		: status == Log::NothingLogged
			? IDS_OPENLOG_EMPTY
			: IDS_OPENLOG_ERROR
	);

	updater.EnableItem(ID_LOG, status == Log::Ok || status == Log::NothingLogged);
	updater.CheckItem(ID_LOG, Log::GetLevel() != spdlog::level::off);

	updater.EnableItem(ID_DESKTOP_COLOR, cfg.DesktopAppearance.Accent != ACCENT_NORMAL);
	EnableAppearanceColor(updater, ID_VISIBLE_COLOR, cfg.VisibleWindowAppearance);
	EnableAppearanceColor(updater, ID_MAXIMISED_COLOR, cfg.MaximisedWindowAppearance);
	EnableAppearanceColor(updater, ID_START_COLOR, cfg.StartOpenedAppearance);
	EnableAppearanceColor(updater, ID_CORTANA_COLOR, cfg.CortanaOpenedAppearance);
	EnableAppearanceColor(updater, ID_TIMELINE_COLOR, cfg.TimelineOpenedAppearance);

	// Block until it finishes
	if (UWP::HasPackageIdentity())
	{
		const auto state = co_await task;
		updater.EnableItem(ID_AUTOSTART,
			!(state == Autostart::StartupState::DisabledByUser || state == Autostart::StartupState::DisabledByPolicy || state == Autostart::StartupState::EnabledByPolicy));
		updater.CheckItem(ID_AUTOSTART, state == Autostart::StartupState::Enabled || state == Autostart::StartupState::EnabledByPolicy);

		uint16_t autostart_text;
		switch (state)
		{
		case Autostart::StartupState::DisabledByUser:
			autostart_text = IDS_AUTOSTART_DISABLED_TASKMGR;
			break;
		case Autostart::StartupState::DisabledByPolicy:
			autostart_text = IDS_AUTOSTART_DISABLED_GPEDIT;
			break;
		case Autostart::StartupState::EnabledByPolicy:
			autostart_text = IDS_AUTOSTART_ENABLED_GPEDIT;
			break;
		case Autostart::StartupState::Enabled:
		case Autostart::StartupState::Disabled:
			autostart_text = IDS_AUTOSTART_NORMAL;
		}
		updater.SetText(ID_AUTOSTART, autostart_text);
	}
}
*/

bool OpenOrCreateMutex(wil::unique_mutex& mutex, const wchar_t* name)
{
	if (mutex.try_open(name))
	{
		return false;
	}
	else
	{
		mutex.create(name);
		return true;
	}
}

void InitializeWindowsRuntime() try
{
	winrt::init_apartment(winrt::apartment_type::single_threaded);
}
HresultErrorCatch(spdlog::level::critical, L"Initialization of Windows Runtime failed.");

std::filesystem::path GetConfigFileLocation(bool hasPackageIdentity)
{
	std::filesystem::path config_folder;
	if (hasPackageIdentity)
	{
		try
		{
			using namespace winrt::Windows::Storage;
			config_folder = std::wstring_view(ApplicationData::Current().RoamingFolder().Path());
		}
		HresultErrorCatch(spdlog::level::critical, L"Getting application folder paths failed!");
	}
	else
	{
		config_folder = win32::GetExeLocation().parent_path();
	}

	config_folder /= CONFIG_FILE;
	return config_folder;
}

bool CheckAndRunWelcome(const std::filesystem::path& config_file, HINSTANCE hInst)
{
	if (!std::filesystem::is_regular_file(config_file))
	{
		Config{ }.Save(config_file);
		if (!WelcomeDialog(config_file, hInst).Run())
		{
			std::filesystem::remove(config_file);
			return false;
		}
	}

	// Remove old version config once prompt is accepted.
	std::filesystem::remove_all(config_file.parent_path() / APP_NAME);

	return true;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ wchar_t *, _In_ int)
{
	win32::HardenProcess();

	wil::unique_mutex mutex;
	if (!OpenOrCreateMutex(mutex, MUTEX_GUID))
	{
		// If there already is another instance running, tell it to exit
		MainAppWindow::CloseRemote();
	}

	InitializeWindowsRuntime();

	const bool hasPackageIdentity = UWP::HasPackageIdentity();
	auto config_file = GetConfigFileLocation(hasPackageIdentity);

	// If the configuration files don't exist, restore the files and show welcome to the users
	if (!CheckAndRunWelcome(config_file, hInstance))
	{
		return EXIT_FAILURE;
	}

	// Initialize GUI
	MainAppWindow window(std::move(config_file), hasPackageIdentity, hInstance);

	// Run the main program loop. When this method exits, TranslucentTB itself is about to exit.
	return static_cast<int>(window.Run());
	// Not uninitializing WinRT apartment here because it will cause issues
	// with destruction of WinRT objects that have a static lifetime.
	// Apartment gets cleaned up by system anyways when the process dies.
}

#pragma endregion
