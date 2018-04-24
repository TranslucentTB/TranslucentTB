#include "config.hpp"
#include <fstream>

#include "common.h"
#include "ttblog.hpp"
#include "util.hpp"
#include "win32.hpp"

// Defaults
swca::ACCENT Config::TASKBAR_APPEARANCE = swca::ACCENT_ENABLE_BLURBEHIND;
uint32_t Config::TASKBAR_COLOR = 0x00000000;
bool Config::DYNAMIC_WS = false;
swca::ACCENT Config::DYNAMIC_APPEARANCE = swca::ACCENT_ENABLE_BLURBEHIND;
uint32_t Config::DYNAMIC_COLOR = 0x00000000;
bool Config::DYNAMIC_NORMAL_ON_PEEK = true;
bool Config::DYNAMIC_START = false;
decltype(Config::PEEK) Config::PEEK = PEEK::Enabled;

uint8_t Config::SLEEP_TIME = 10;
uint16_t Config::CACHE_HIT_MAX = 500;
bool Config::VERBOSE =
#ifndef _DEBUG
	false;
#else
	true;
#endif

void Config::Parse(const std::wstring &file)
{
	std::wifstream configstream(file);

	for (std::wstring line; std::getline(configstream, line);)
	{
		if (line.empty())
		{
			continue;
		}

		// Skip comments
		size_t comment_index = line.find(L';');
		if (comment_index == 0)
		{
			continue;
		}
		else if (comment_index != std::wstring::npos)
		{
			line = line.substr(0, comment_index);
		}

		size_t split_index = line.find(L'=');
		if (split_index != std::wstring::npos)
		{
			std::wstring key = line.substr(0, split_index);
			std::wstring val = line.substr(split_index + 1, line.length() - split_index - 1);
			ParseSingleConfigOption(key, val);
		}
		else
		{
			Log::OutputMessage(L"Invalid line in configuration file: " + line);
		}
	}
}

void Config::UnknownValue(const std::wstring & key, const std::wstring & value)
{
	Log::OutputMessage(L"Unknown value found in configuration file: " + value + L" (for key: " + key + L")");
}

bool Config::ParseAccent(const std::wstring &value, swca::ACCENT &accent)
{
	if (value == L"blur")
	{
		accent = swca::ACCENT_ENABLE_BLURBEHIND;
	}
	else if (value == L"opaque")
	{
		accent = swca::ACCENT_ENABLE_GRADIENT;
	}
	else if (value == L"transparent" || value == L"translucent" || value == L"clear")
	{
		accent = swca::ACCENT_ENABLE_TRANSPARENTGRADIENT;
	}
	else if (value == L"normal")
	{
		accent = swca::ACCENT_NORMAL;
	}
	else if (value == L"fluent" && win32::IsAtLeastBuild(MIN_FLUENT_BUILD))
	{
		accent = swca::ACCENT_ENABLE_FLUENT;
	}
	else
	{
		return false;
	}

	return true;
}

bool Config::ParseColor(const std::wstring &value, uint32_t &color)
{
	std::wstring color_value = Util::Trim(value);

	if (color_value.find(L'#') == 0)
	{
		color_value = color_value.substr(1, color_value.length() - 1);
	}
	else if (color_value.find(L"0x") == 0)
	{
		color_value = color_value.substr(2, color_value.length() - 2);
	}

	// Get only the last 6 characters, keeps compatibility with old version.
	// It stored AARRGGBB in color, but now we store it as RRGGBB.
	// We read AA from opacity instead, which the old version also saved alpha to.
	if (color_value.length() > 6)
	{
		color_value = color_value.substr(color_value.length() - 6, 6);
	}

	try
	{
		color = (color & 0xFF000000) + (std::stoi(color_value, nullptr, 16) & 0x00FFFFFF);
	}
	catch (std::invalid_argument)
	{
		return false;
	}

	return true;
}

bool Config::ParseOpacity(const std::wstring & value, uint32_t & color)
{
	try
	{
		((std::stoi(value) & 0xFF) << 24) + (color & 0x00FFFFFF);
		return true;
	}
	catch (std::invalid_argument)
	{
		Log::OutputMessage(L"Could not parse opacity found in configuration file: " + value);
		return false;
	}
}

bool Config::ParseBool(const std::wstring &value, bool &setting)
{
	if (value == L"true" || value == L"enable")
	{
		setting = true;
	}
	else if (value == L"false" || value == L"disable")
	{
		setting = false;
	}
	else
	{
		return false;
	}

	return true;
}

void Config::ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value)
{
	if (arg == L"accent")
	{
		if (!ParseAccent(value, TASKBAR_APPEARANCE))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"color" || arg == L"tint")
	{
		if (!ParseColor(value, TASKBAR_COLOR))
		{
			Log::OutputMessage(L"Could not parse color found in configuration file: " + value);
		}
	}
	else if (arg == L"opacity")
	{
		if (!ParseOpacity(value, TASKBAR_COLOR))
		{
			Log::OutputMessage(L"Could not parse opacity found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws")
	{
		if (!ParseBool(value, DYNAMIC_WS))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-ws-accent")
	{
		if (!ParseAccent(value, DYNAMIC_APPEARANCE))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-ws-color" || arg == L"dynamic-ws-tint")
	{
		if (!ParseColor(value, DYNAMIC_COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic windows color found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws-opacity")
	{
		if (!ParseOpacity(value, DYNAMIC_COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic windows opacity found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws-normal-on-peek")
	{
		if (!ParseBool(value, DYNAMIC_NORMAL_ON_PEEK))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-start")
	{
		if (!ParseBool(value, DYNAMIC_START))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"peek")
	{
		if (value == L"hide")
		{
			PEEK = PEEK::Disabled;
		}
		else if (value == L"dynamic")
		{
			PEEK = PEEK::Dynamic;
		}
		else if (value == L"show")
		{
			PEEK = PEEK::Enabled;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"sleep-time")
	{
		try
		{
			SLEEP_TIME = std::stoi(value) & 0xFF;
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse sleep time found in configuration file: " + value);
		}
	}
	else if (arg == L"max-cache-hits")
	{
		try
		{
			CACHE_HIT_MAX = std::stoi(value) & 0xFFFF;
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse max cache hits found in configuration file: " + value);
		}
	}
	else if (arg == L"verbose")
	{
		if (!ParseBool(value, Config::VERBOSE))
		{
			UnknownValue(arg, value);
		}
	}
	else
	{
		Log::OutputMessage(L"Unknown key found in configuration file: " + arg);
	}
}