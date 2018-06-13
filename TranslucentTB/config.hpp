#pragma once
#include <cstdint>
#include <string>

#include "swcadata.hpp"

class Config {

public:
	typedef struct {
		swca::ACCENT ACCENT;
		uint32_t     COLOR;
	} TASKBAR_APPEARANCE;

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
	static bool VERBOSE;

	static void Parse(const std::wstring &file);
	static void Save(const std::wstring &file);

private:
	static void UnknownValue(const std::wstring &key, const std::wstring &value);
	static bool ParseAccent(const std::wstring &value, swca::ACCENT &accent);
	static bool ParseColor(std::wstring value, uint32_t &color);
	static bool ParseOpacity(const std::wstring &value, uint32_t &color);
	static bool ParseBool(const std::wstring &value, bool &setting);
	static void ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value);

	static std::wstring GetAccentText(const swca::ACCENT &accent);
	static std::wstring GetColorText(const uint32_t &color);
	static std::wstring GetOpacityText(const uint32_t &color);
	static std::wstring GetBoolText(const bool &value);
};