#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "swcadata.hpp"

class Config {

public:
	struct TASKBAR_APPEARANCE {
		swca::ACCENT ACCENT;
		uint32_t     COLOR;
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
	static bool VERBOSE;

	static void Parse(const std::wstring &file);
	static bool ParseCommandLine();
	static void Save(const std::wstring &file);

private:
	static std::mutex m_ConfigLock;
	static const std::wstring CLI_HELP_MSG;
	static const std::pair<const std::wstring_view, bool &> CLI_FLAGS[9];

	static std::vector<std::wstring> GetArgs();

	static void UnknownValue(const std::wstring &key, const std::wstring &value, const std::function<void(const std::wstring &)> &logger);
	static bool ParseAccent(const std::wstring &value, swca::ACCENT &accent);
	static bool ParseColor(std::wstring value, uint32_t &color);
	static bool ParseOpacity(const std::wstring &value, uint32_t &color);
	static bool ParseBool(const std::wstring &value, bool &setting);
	static void ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger);

	static std::wstring GetAccentText(const swca::ACCENT &accent);
	static std::wstring GetColorText(const uint32_t &color);
	static std::wstring GetOpacityText(const uint32_t &color);
	static std::wstring GetBoolText(const bool &value);
};