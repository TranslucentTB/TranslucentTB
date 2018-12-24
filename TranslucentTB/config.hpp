#pragma once
#include <cstdint>
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
	static enum /*class*/ PEEK {
		Disabled, // Hide the button
		Dynamic,  // Show when a window is maximised
		Enabled   // Don't hide the button
	} PEEK;
	static bool PEEK_ONLY_MAIN;

	// Advanced
	static uint8_t SLEEP_TIME;
	static bool NO_TRAY;
	static bool NO_SAVE;
	static bool NO_HOOK;
	static bool VERBOSE;

	static void Parse(const std::wstring &file);
	static bool ParseCommandLine();
	static void Save(const std::wstring &file);

private:
	static const std::wstring CLI_HELP_MSG;
	static const std::pair<const std::wstring_view, bool &> FLAGS[10];
	static const std::pair<const std::wstring_view, TASKBAR_APPEARANCE &> APPEARANCES[5];

	static std::vector<std::wstring> GetArgs();

	static void UnknownValue(const std::wstring &key, const std::wstring &value, const std::function<void(const std::wstring &)> &logger);
	static bool ParseAccent(const std::wstring &value, ACCENT_STATE &accent);
	static bool ParseColor(std::wstring value, COLORREF &color);
	static bool ParseOpacity(const std::wstring &value, COLORREF &color);
	static bool ParseBool(const std::wstring &value, bool &setting);
	static void ParseKeyValuePair(std::wstring kvp);
	static bool ParseFlags(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger);
	static void ParseCliFlags(std::vector<std::wstring> &args, const std::function<void(const std::wstring &)> &logger);
	static bool ParseAppearances(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger);
	static void ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger);

	static std::wstring GetAccentText(ACCENT_STATE accent);
	static std::wstring GetColorText(uint32_t color);
	static std::wstring GetOpacityText(uint32_t color);
	static std::wstring GetBoolText(bool value);
};