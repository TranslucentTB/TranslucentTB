#pragma once
#include <cstdint>
#include <string>

#include "swcadata.hpp"

class Config {

public:
	// Regular
	static swca::ACCENT REGULAR_APPEARANCE;
	static uint32_t REGULAR_COLOR;

	// Maximised
	static bool MAXIMISED_ENABLED;
	static swca::ACCENT MAXIMISED_APPEARANCE;
	static uint32_t MAXIMISED_COLOR;
	static bool MAXIMISED_REGULAR_ON_PEEK;

	// Start menu
	static bool START_ENABLED;
	static swca::ACCENT START_APPEARANCE;
	static uint32_t START_COLOR;

	// Various
	static enum /*class*/ PEEK {
		Disabled,		// Hide the button
		Dynamic,		// Show when a window is maximised
		Enabled			// Don't hide the button
	} PEEK;

	// Advanced
	static uint8_t SLEEP_TIME;
	static bool VERBOSE;

	static void Parse(const std::wstring &file);
	static void Save(const std::wstring &file);

private:
	static void UnknownValue(const std::wstring &key, const std::wstring &value);
	static bool ParseAccent(const std::wstring &value, swca::ACCENT &accent);
	static bool ParseColor(const std::wstring &value, uint32_t &color);
	static bool ParseOpacity(const std::wstring &value, uint32_t &color);
	static bool ParseBool(const std::wstring &value, bool &setting);
	static void ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value);

	static std::wstring GetAccentText(const swca::ACCENT &accent);
	static std::wstring GetColorText(const uint32_t &color);
	static std::wstring GetOpacityText(const uint32_t &color);
	static std::wstring GetBoolText(const bool &value);
};