#pragma once
#include <cstdint>
#include <string>

#include "swcadata.hpp"

class Config {

public:
	static swca::ACCENT TASKBAR_APPEARANCE;
	static uint32_t TASKBAR_COLOR;
	static bool DYNAMIC_WS;
	static swca::ACCENT DYNAMIC_APPEARANCE;
	static uint32_t DYNAMIC_COLOR;
	static bool DYNAMIC_REGULAR_ON_PEEK;
	static bool DYNAMIC_USE_REGULAR_COLOR;
	static bool DYNAMIC_START;
	static enum /*class*/ PEEK {
		Disabled,		// Hide the button
		Dynamic,		// Show when a window is maximised
		Enabled			// Don't hide the button
	} PEEK;

	static uint8_t SLEEP_TIME;
	static uint16_t CACHE_HIT_MAX;
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