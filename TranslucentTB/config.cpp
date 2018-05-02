#include "config.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

#include "common.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#include "win32.hpp"

// Defaults
swca::ACCENT Config::TASKBAR_APPEARANCE = swca::ACCENT::ACCENT_ENABLE_BLURBEHIND;
uint32_t Config::TASKBAR_COLOR = 0x00000000;
bool Config::DYNAMIC_WS = false;
swca::ACCENT Config::DYNAMIC_APPEARANCE = swca::ACCENT::ACCENT_ENABLE_BLURBEHIND;
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

void Config::Save(const std::wstring &file)
{
	using namespace std;
	wofstream configstream(file);

	configstream << L"; Taskbar appearance: clear, normal, fluent (only on build " << MIN_FLUENT_BUILD << L" and up), opaque, normal, or blur (default)." << endl;
	configstream << L"accent=" << GetAccentText(TASKBAR_APPEARANCE) << endl;
	configstream << L"; Color and opacity of the taskbar." << endl;
	configstream << L"color=" << GetColorText(TASKBAR_COLOR) << L" ; A color in hexadecimal notation." << endl;
	configstream << L"opacity=" << GetOpacityText(TASKBAR_COLOR) << L"  ; A value in the range 0 to 255." << endl;

	configstream << endl;
	configstream << L"; Dynamic Windows and Start Menu" << endl;
	configstream << L"; Available states are the same as the accent configuration option." << endl;
	configstream << L"; dynamic windows has its own color and opacity configs." << endl;
	configstream << L"; by enabling dynamic-ws-normal-on-peek, dynamic windows will behave as if no window is maximised when using Aero Peek." << endl;
	configstream << L"; you can also set the accent, color and opacity values, which will represent the state of dynamic windows when there is no window maximised." << endl;
	configstream << L"; dynamic start returns the taskbar to normal appearance when the start menu is opened." << endl;
	configstream << L"dynamic-ws=" << GetBoolText(DYNAMIC_WS) << endl;
	configstream << L"dynamic-ws-accent=" << GetAccentText(DYNAMIC_APPEARANCE) << endl;
	configstream << L"dynamic-ws-color=" << GetColorText(DYNAMIC_COLOR) << L" ; A color in hexadecimal notation." << endl;
	configstream << L"dynamic-ws-opacity=" << GetOpacityText(DYNAMIC_COLOR) << L"  ; A value in the range 0 to 255." << endl;
	configstream << L"dynamic-ws-normal-on-peek=" << GetBoolText(DYNAMIC_NORMAL_ON_PEEK) << endl;
	configstream << L"dynamic-start=" << GetBoolText(DYNAMIC_START) << endl;

	configstream << endl;
	configstream << L"; Controls how the Aero Peek button behaves (dynamic, show or hide)" << endl;
	configstream << L"peek=";
	switch (PEEK)
	{
	case PEEK::Disabled:
		configstream << L"hide";
		break;
	case PEEK::Dynamic:
		configstream << L"dynamic";
		break;
	case PEEK::Enabled:
		configstream << L"show";
		break;
	}
	configstream << endl;

	configstream << endl;
	configstream << L"; Advanced settings" << endl;
	configstream << L"; sleep time in milliseconds, a shorter time reduces flicker when opening start, but results in higher CPU usage." << endl;
	configstream << L"sleep-time=" << std::dec << SLEEP_TIME << endl;
	configstream << L"; maximum number of times the blacklist cache can be hit before getting cleared." << endl;
	configstream << L"; (ignored when no window titles are blacklisted as the cache never clears automatically in that case)" << endl;
	configstream << L"max-cache-hits=" << std::dec << CACHE_HIT_MAX << endl;
	configstream << L"; more informative logging. Can make huge log files." << endl;
	configstream << L"verbose=" << GetBoolText(VERBOSE) << endl;
}

void Config::UnknownValue(const std::wstring & key, const std::wstring & value)
{
	Log::OutputMessage(L"Unknown value found in configuration file: " + value + L" (for key: " + key + L")");
}

bool Config::ParseAccent(const std::wstring &value, swca::ACCENT &accent)
{
	if (value == L"blur")
	{
		accent = swca::ACCENT::ACCENT_ENABLE_BLURBEHIND;
	}
	else if (value == L"opaque")
	{
		accent = swca::ACCENT::ACCENT_ENABLE_GRADIENT;
	}
	else if (value == L"transparent" || value == L"translucent" || value == L"clear")
	{
		accent = swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT;
	}
	else if (value == L"normal")
	{
		accent = swca::ACCENT::ACCENT_NORMAL;
	}
	else if (value == L"fluent" && win32::IsAtLeastBuild(MIN_FLUENT_BUILD))
	{
		accent = swca::ACCENT::ACCENT_ENABLE_FLUENT;
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

bool Config::ParseOpacity(const std::wstring &value, uint32_t &color)
{
	try
	{
		color = ((std::stoi(value) & 0xFF) << 24) + (color & 0x00FFFFFF);
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

std::wstring Config::GetAccentText(const swca::ACCENT &accent)
{
	switch (accent)
	{
	case swca::ACCENT::ACCENT_ENABLE_GRADIENT:
		return L"opaque";
	case swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT:
		return L"clear";
	case swca::ACCENT::ACCENT_ENABLE_BLURBEHIND:
		return L"blur";
	case swca::ACCENT::ACCENT_NORMAL:
		return L"normal";
	case swca::ACCENT::ACCENT_ENABLE_FLUENT:
		return L"fluent";
	default:
		throw std::invalid_argument("accent was not one of the known values");
	}
}

std::wstring Config::GetColorText(const uint32_t &color)
{
	std::wostringstream stream;
	stream << std::right << std::setw(6) << std::setfill(L'0') << std::hex << (color & 0x00FFFFFF);
	return stream.str();
}

std::wstring Config::GetOpacityText(const uint32_t &color)
{
	std::wostringstream stream;
	stream << std::left << std::setw(3) << std::setfill(L' ') << std::dec << ((color & 0xFF000000) >> 24);
	return stream.str();
}

std::wstring Config::GetBoolText(const bool &value)
{
	return value ? L"enable" : L"disable";
}
