#include "config.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "smart/autofree.hpp"
#include "constants.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#include "win32.hpp"
#include "windows/window.hpp"

// Defaults

// Regular
Config::TASKBAR_APPEARANCE Config::REGULAR_APPEARANCE = { ACCENT_ENABLE_TRANSPARENTGRADIENT, 0x0 };

// Maximised
bool Config::MAXIMISED_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::MAXIMISED_APPEARANCE = { ACCENT_ENABLE_BLURBEHIND, 0xaa000000 };
bool Config::MAXIMISED_REGULAR_ON_PEEK = true;

// Start
bool Config::START_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::START_APPEARANCE = { ACCENT_NORMAL, 0x0 };

// Cortana
bool Config::CORTANA_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::CORTANA_APPEARANCE = { ACCENT_NORMAL, 0x0 };

// Timeline/Task View
bool Config::TIMELINE_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::TIMELINE_APPEARANCE = { ACCENT_NORMAL, 0x0 };

// Peek
enum Config::PEEK Config::PEEK = PEEK::DynamicMainMonitor;

// Advanced
uint8_t Config::SLEEP_TIME = 10;
bool Config::NO_TRAY = false;
bool Config::NO_SAVE = false;
bool Config::NO_HOOK = false;
bool Config::VERBOSE =
#ifndef _DEBUG
	false;
#else
	true;
#endif

const std::wstring Config::CLI_HELP_MSG =
LR"(Flags (can be alone or take one of true or false):
	--dynamic-ws
	--dynamic-ws-regular-on-peek
	--dynamic-start
	--dynamic-cortana
	--dynamic-timeline
	--no-tray
	--no-save
	--no-hook
	--verbose

Accents (takes one of opaque, clear, blur, normal or fluent):
	--accent
	--dynamic-ws-accent
	--dynamic-start-accent
	--dynamic-cortana-accent
	--dynamic-timeline-accent

Colors (takes a color in the format RRGGBB):
	--color
	--dynamic-ws-color
	--dynamic-start-color
	--dynamic-cortana-color
	--dynamic-timeline-color

Opacity (takes an integral number between 0 and 255):
	--opacity
	--dynamic-ws-opacity
	--dynamic-start-opacity
	--dynamic-cortana-opacity
	--dynamic-timeline-opacity

Others:
	--help (shows this menu and exits)
	--peek
	--sleep-time

TranslucentTB accepts parameters in the following format:
	--argument value
	--flag

See configuration file for details.)";

const std::pair<const std::wstring_view, bool &> Config::FLAGS[] = {
	{ L"dynamic-ws", MAXIMISED_ENABLED },
	{ L"dynamic-ws-regular-on-peek", MAXIMISED_REGULAR_ON_PEEK },
	{ L"dynamic-start", START_ENABLED },
	{ L"dynamic-cortana", CORTANA_ENABLED },
	{ L"dynamic-timeline", TIMELINE_ENABLED },
	{ L"no-tray", NO_TRAY },
	{ L"no-save", NO_SAVE },
	{ L"no-hook", NO_HOOK },
	{ L"verbose", VERBOSE }
};

const std::pair<const std::wstring_view, Config::TASKBAR_APPEARANCE &> Config::APPEARANCES[] = {
	{ L"", REGULAR_APPEARANCE },
	{ L"dynamic-ws-", MAXIMISED_APPEARANCE },
	{ L"dynamic-start-", START_APPEARANCE },
	{ L"dynamic-cortana-", CORTANA_APPEARANCE },
	{ L"dynamic-timeline-", TIMELINE_APPEARANCE }
};

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
			line.erase(comment_index);
		}

		ParseKeyValuePair(std::move(line));
	}
}

bool Config::ParseCommandLine()
{
	std::vector<std::wstring> args = GetArgs();
	if (args.empty())
	{
		return true;
	}

	std::for_each(args.begin(), args.end(), Util::ToLowerInplace);
	std::for_each(args.begin(), args.end(), std::bind(&Util::TrimInplace, std::placeholders::_1, L' '));

	if (std::find(args.begin(), args.end(), L"--help") != args.end())
	{
		// TODO: use a TaskDialog with links to open config files?
		MessageBox(Window::NullWindow, CLI_HELP_MSG.c_str(), NAME, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
		return false;
	}
	else
	{
		// note: std::vector::erase's last item is non-inclusive, hence the use of + 2 over + 1

		std::wostringstream output;
		auto logger = [&output](const std::wstring &text)
		{
			output << text << std::endl;
		};

		ParseCliFlags(args, logger);

		while (args.size() >= 2)
		{
			if (!Util::StringBeginsWith(args[1], L"--"))
			{
				Util::RemovePrefixInplace(args[0], L"--");

				ParseSingleConfigOption(args[0], args[1], logger);
				args.erase(args.begin(), args.begin() + 2);
			}
			else
			{
				output << L"Missing value for CLI option " << args[0] << std::endl;
				args.erase(args.begin());
			}
		}

		if (!args.empty())
		{
			output << L"Orphaned CLI option detected: " << args[0] << std::endl;
		}

		std::wstring out = output.str();
		if (!out.empty())
		{
			MessageBox(Window::NullWindow, out.c_str(), NAME, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
		}

		return true;
	}
}

void Config::Save(const std::wstring &file)
{
	if (NO_SAVE)
	{
		return;
	}

	std::wofstream configstream(file);

	configstream << L"accent=" << std::left << std::setw(6) << std::setfill(L' ') << GetAccentText(REGULAR_APPEARANCE.ACCENT) << L"; accent values are: clear (default), fluent, opaque, normal, or blur." << std::endl;
	configstream << L"color=" << GetColorText(REGULAR_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << std::endl;
	configstream << L"opacity=" << GetOpacityText(REGULAR_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << std::endl;

	configstream << std::endl;
	configstream << L"; Dynamic Modes" << std::endl;
	configstream << L"; they all have their own accent, color and opacity configs." << std::endl;
	configstream << std::endl;
	configstream << L"; Dynamic Windows. State to use when a window is maximised." << std::endl;
	configstream << L"dynamic-ws=" << GetBoolText(MAXIMISED_ENABLED) << std::endl;
	configstream << L"dynamic-ws-accent=" << GetAccentText(MAXIMISED_APPEARANCE.ACCENT) << std::endl;
	configstream << L"dynamic-ws-color=" << GetColorText(MAXIMISED_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << std::endl;
	configstream << L"dynamic-ws-opacity=" << GetOpacityText(MAXIMISED_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << std::endl;
	configstream << L"dynamic-ws-regular-on-peek=" << GetBoolText(MAXIMISED_REGULAR_ON_PEEK) << L" ; when using aero peek, behave as if no window was maximised." << std::endl;
	configstream << std::endl;
	configstream << L"; Dynamic Start. State to use when the start menu is opened." << std::endl;
	configstream << L"dynamic-start=" << GetBoolText(START_ENABLED) << std::endl;
	configstream << L"dynamic-start-accent=" << GetAccentText(START_APPEARANCE.ACCENT) << std::endl;
	configstream << L"dynamic-start-color=" << GetColorText(START_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << std::endl;
	configstream << L"dynamic-start-opacity=" << GetOpacityText(START_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << std::endl;
	configstream << std::endl;
	configstream << L"; Dynamic Cortana. State to use when Cortana or the search menu is opened." << std::endl;
	configstream << L"dynamic-cortana=" << GetBoolText(CORTANA_ENABLED) << std::endl;
	configstream << L"dynamic-cortana-accent=" << GetAccentText(CORTANA_APPEARANCE.ACCENT) << std::endl;
	configstream << L"dynamic-cortana-color=" << GetColorText(CORTANA_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << std::endl;
	configstream << L"dynamic-cortana-opacity=" << GetOpacityText(CORTANA_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << std::endl;
	configstream << std::endl;
	configstream << L"; Dynamic Timeline. State to use when the timeline (or task view on older builds) is opened." << std::endl;
	configstream << L"dynamic-timeline=" << GetBoolText(TIMELINE_ENABLED) << std::endl;
	configstream << L"dynamic-timeline-accent=" << GetAccentText(TIMELINE_APPEARANCE.ACCENT) << std::endl;
	configstream << L"dynamic-timeline-color=" << GetColorText(TIMELINE_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << std::endl;
	configstream << L"dynamic-timeline-opacity=" << GetOpacityText(TIMELINE_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << std::endl;

	configstream << std::endl;
	configstream << L"; Controls how the Aero Peek button behaves (dynamic, show or hide)" << std::endl;
	configstream << L"peek=";
	switch (PEEK)
	{
	case PEEK::Disabled:
		configstream << L"hide";
		break;
	case PEEK::DynamicMainMonitor:
		configstream << L"dynamic-main-monitor";
		break;
	case PEEK::DynamicAnyMonitor:
		configstream << L"dynamic-any-monitor";
		break;
	case PEEK::DynamicDesktopForeground:
		configstream << L"dynamic-desktop-foreground";
		break;
	case PEEK::Enabled:
		configstream << L"show";
		break;
	}
	configstream << std::endl;

	configstream << std::endl;
	configstream << L"; Advanced settings" << std::endl;
	configstream << L"; sleep time in milliseconds, a shorter time reduces flicker when opening start, but results in higher CPU usage." << std::endl;
	configstream << L"sleep-time=" << std::dec << SLEEP_TIME << std::endl;
	configstream << L"; hide icon in system tray. Changes to this requires a restart of the application." << std::endl;
	configstream << L"no-tray=" << GetBoolText(NO_TRAY) << std::endl;
	configstream << L"; don't save the configuration file. Prevents the program from writing to the config file at all." << std::endl;
	configstream << L"no-save=" << GetBoolText(NO_SAVE) << std::endl;
	configstream << L"; disable Windows Explorer hooking. Use if you are having issues with it (eg frequent explorer crashes). Needs a restart of the program to apply." << std::endl;
	configstream << L"no-hook=" << GetBoolText(NO_HOOK) << std::endl;
	configstream << L"; more informative logging. Can make huge log files." << std::endl;
	configstream << L"verbose=" << GetBoolText(VERBOSE) << std::endl;
}

std::vector<std::wstring> Config::GetArgs()
{
	int argc;
	const AutoFree::Local<wchar_t *[]> argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (!argv)
	{
		LastErrorHandle(Error::Level::Error, L"Failed to convert command line to argument vector. CLI arguments won't be parsed.");
		return { };
	}
	else
	{
		return { argv.get() + 1, argv.get() + argc };
	}
}

void Config::UnknownValue(const std::wstring &key, const std::wstring &value, const std::function<void(const std::wstring &)> &logger)
{
	logger(L"Unknown value found: " + value + L" (for key: " + key + L')');
}

bool Config::ParseAccent(const std::wstring &value, ACCENT_STATE &accent)
{
	if (value == L"blur")
	{
		accent = ACCENT_ENABLE_BLURBEHIND;
	}
	else if (value == L"opaque")
	{
		accent = ACCENT_ENABLE_GRADIENT;
	}
	else if (value == L"transparent" || value == L"translucent" || value == L"clear")
	{
		accent = ACCENT_ENABLE_TRANSPARENTGRADIENT;
	}
	else if (value == L"normal")
	{
		accent = ACCENT_NORMAL;
	}
	else if (value == L"fluent")
	{
		accent = ACCENT_ENABLE_ACRYLICBLURBEHIND;
	}
	else
	{
		return false;
	}

	return true;
}

bool Config::ParseColor(std::wstring value, COLORREF &color)
{
	Util::TrimInplace(value);

	Util::RemovePrefixInplace(value, L"#");
	Util::RemovePrefixInplace(value, L"0x");

	// Get only the last 6 characters, keeps compatibility with old version.
	// It stored AARRGGBB in color, but now we store it as RRGGBB.
	// We read AA from opacity instead, which the old version also saved alpha to.
	if (value.length() > 6)
	{
		value.erase(0, 2);
	}

	try
	{
		color = (color & 0xFF000000) + (std::stoi(value, nullptr, 16) & 0x00FFFFFF);
	}
	catch (...)
	{
		return false;
	}

	return true;
}

bool Config::ParseOpacity(const std::wstring &value, COLORREF &color)
{
	try
	{
		color = ((std::stoi(value) & 0xFF) << 24) + (color & 0x00FFFFFF);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool Config::ParseBool(const std::wstring &value, bool &setting)
{
	if (value == L"true" || value == L"enable" || value == L"enabled" )
	{
		setting = true;
	}
	else if (value == L"false" || value == L"disable" || value == L"disabled")
	{
		setting = false;
	}
	else
	{
		return false;
	}

	return true;
}

void Config::ParseKeyValuePair(std::wstring kvp)
{
	size_t split_index = kvp.find(L'=');
	if (split_index != std::wstring::npos)
	{
		Util::ToLowerInplace(kvp);
		std::wstring_view line_view = kvp;
		std::wstring_view key = Util::Trim(line_view.substr(0, split_index));
		std::wstring_view val = Util::Trim(line_view.substr(split_index + 1, line_view.length() - split_index - 1));

		ParseSingleConfigOption(std::wstring(key), std::wstring(val), Log::OutputMessage);
	}
	else
	{
		Log::OutputMessage(L"Invalid line in configuration file: " + kvp);
	}
}

bool Config::ParseFlags(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger)
{
	for (const auto &pair : FLAGS)
	{
		if (arg == pair.first)
		{
			if (!ParseBool(value, pair.second))
			{
				UnknownValue(arg, value, logger);
			}

			return true;
		}
	}

	return false;
}

void Config::ParseCliFlags(std::vector<std::wstring> &args, const std::function<void(const std::wstring &)> &logger)
{
	for (const auto &pair : FLAGS)
	{
		auto iter = std::find(args.begin(), args.end(), L"--" + std::wstring(pair.first));
		if (iter != args.end())
		{
			if (iter + 1 != args.end() && !Util::StringBeginsWith(*(iter + 1), L"--"))
			{
				if (!ParseBool(*(iter + 1), pair.second))
				{
					UnknownValue(std::wstring(Util::RemovePrefix(*iter, L"--")), *(iter + 1), logger);
				}

				args.erase(iter, iter + 2);
			}
			else
			{
				pair.second = true;
				args.erase(iter);
			}
		}
	}
}

bool Config::ParseAppearances(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger)
{
	for (const auto &pair : APPEARANCES)
	{
		std::wstring prefix(pair.first);
		if (arg == prefix + L"accent")
		{
			if (!ParseAccent(value, pair.second.ACCENT))
			{
				UnknownValue(arg, value, logger);
			}

			return true;
		}
		else if (arg == prefix + L"color" || arg == prefix + L"tint")
		{
			if (!ParseColor(value, pair.second.COLOR))
			{
				UnknownValue(arg, value, logger);
			}

			return true;
		}
		else if (arg == prefix + L"opacity")
		{
			if (!ParseOpacity(value, pair.second.COLOR))
			{
				UnknownValue(arg, value, logger);
			}

			return true;
		}
	}

	return false;
}

void Config::ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger)
{
	if (ParseFlags(arg, value, logger))
	{
		return;
	}

	if (ParseAppearances(arg, value, logger))
	{
		return;
	}

	if (arg == L"peek")
	{
		if (value == L"hide")
		{
			PEEK = PEEK::Disabled;
		}
		else if (value == L"dynamic" || value == L"dynamic-main-monitor")
		{
			PEEK = PEEK::DynamicMainMonitor;
		}
		else if (value == L"dynamic-any-monitor")
		{
			PEEK = PEEK::DynamicAnyMonitor;
		}
		else if (value == L"dynamic-desktop-foreground")
		{
			PEEK = PEEK::DynamicDesktopForeground;
		}
		else if (value == L"show")
		{
			PEEK = PEEK::Enabled;
		}
		else
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"sleep-time")
	{
		try
		{
			SLEEP_TIME = Util::ClampTo<uint8_t>(std::stoi(value));
		}
		catch (...)
		{
			logger(L"Could not parse sleep time: " + value);
		}
	}
	else
	{
		logger(L"Unknown key found: " + arg);
	}
}

std::wstring Config::GetAccentText(ACCENT_STATE accent)
{
	switch (accent)
	{
	case ACCENT_ENABLE_GRADIENT:
		return L"opaque";
	case ACCENT_ENABLE_TRANSPARENTGRADIENT:
		return L"clear";
	case ACCENT_ENABLE_BLURBEHIND:
		return L"blur";
	case ACCENT_NORMAL:
		return L"normal";
	case ACCENT_ENABLE_ACRYLICBLURBEHIND:
		return L"fluent";
	default:
		throw std::invalid_argument("accent was not one of the known values");
	}
}

std::wstring Config::GetColorText(uint32_t color)
{
	std::wostringstream stream;
	stream << std::right << std::setw(6) << std::setfill(L'0') << std::hex << (color & 0x00FFFFFF);
	return stream.str();
}

std::wstring Config::GetOpacityText(uint32_t color)
{
	std::wostringstream stream;
	stream << std::left << std::setw(3) << std::setfill(L' ') << std::dec << ((color & 0xFF000000) >> 24);
	return stream.str();
}

std::wstring Config::GetBoolText(bool value)
{
	return value ? L"enable" : L"disable";
}