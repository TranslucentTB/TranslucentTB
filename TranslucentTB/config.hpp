#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "swcadef.h"

class Config {
public:
	struct TASKBAR_APPEARANCE {
		ACCENT_STATE ACCENT;
		COLORREF     COLOR;
	};

	// Regular
	static TASKBAR_APPEARANCE REGULAR_APPEARANCE;

	// Maximised
	static bool MAXIMISED_ENABLED;
	static TASKBAR_APPEARANCE MAXIMISED_APPEARANCE;
	static bool MAXIMISED_REGULAR_ON_PEEK;

	// Start menu
	static bool START_ENABLED;
	static TASKBAR_APPEARANCE START_APPEARANCE;

	// Cortana
	static bool CORTANA_ENABLED;
	static TASKBAR_APPEARANCE CORTANA_APPEARANCE;

	// Timeline/Task View
	static bool TIMELINE_ENABLED;
	static TASKBAR_APPEARANCE TIMELINE_APPEARANCE;

	// Peek
	static enum class PEEK {
		Disabled,                 // Hide the button
		DynamicMainMonitor,       // Show when a window is maximised on the main monitor
		DynamicAnyMonitor,        // Show when a window is maximised on any monitor
		DynamicDesktopForeground, // Show when the desktop is the foreground window
		Enabled                   // Don't hide the button
	} PEEK;

	// Advanced
	static uint8_t SLEEP_TIME;
	static bool NO_TRAY;
	static bool NO_SAVE;
	static bool NO_HOOK;
	static bool VERBOSE;

	static void Parse(const std::filesystem::path &file);
	static bool ParseCommandLine();
	static void Save(const std::filesystem::path &file);

private:
	using logger_t = std::function<void(std::wstring_view)>;

	static const std::wstring CLI_HELP_MSG;
	static const std::pair<const std::wstring_view, bool &> FLAGS[9];
	static const std::pair<const std::wstring_view, TASKBAR_APPEARANCE &> APPEARANCES[5];

	static std::vector<std::wstring> GetArgs();

	static void UnknownValue(std::wstring_view key, std::wstring_view value, const std::function<void(const std::wstring &)> &logger);
	static bool ParseAccent(std::wstring_view value, ACCENT_STATE &accent);
	static bool ParseColor(std::wstring_view value, COLORREF &color, const logger_t &logger);
	static bool ParseOpacity(std::wstring_view value, COLORREF &color, const logger_t &logger);
	static bool ParseBool(std::wstring_view value, bool &setting);
	static void ParseKeyValuePair(std::wstring_view kvp);
	static bool ParseFlags(std::wstring_view arg, std::wstring_view value, const logger_t &logger);
	static void ParseCliFlags(std::vector<std::wstring> &args, const logger_t &logger);
	static bool ParseAppearances(std::wstring_view arg, std::wstring_view value, const logger_t &logger);
	static void ParseSingleConfigOption(std::wstring_view arg, std::wstring_view value, const logger_t &logger);

	static std::wstring_view GetAccentText(ACCENT_STATE accent);
	static std::wstring GetColorText(uint32_t color);
	static std::wstring GetOpacityText(uint32_t color);
	static std::wstring_view GetBoolText(bool value);
};