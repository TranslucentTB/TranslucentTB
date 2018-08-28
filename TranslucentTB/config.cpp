#include "config.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "autofree.hpp"
#include "common.hpp"
#include "ttberror.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#include "win32.hpp"
#include "window.hpp"

// Defaults

// Regular
Config::TASKBAR_APPEARANCE Config::REGULAR_APPEARANCE = { swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT, 0x0 };

// Maximised
bool Config::MAXIMISED_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::MAXIMISED_APPEARANCE = { swca::ACCENT::ACCENT_ENABLE_BLURBEHIND, 0xaa000000 };
bool Config::MAXIMISED_REGULAR_ON_PEEK = true;

// Start
bool Config::START_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::START_APPEARANCE = { swca::ACCENT::ACCENT_NORMAL, 0x0 };

// Cortana
bool Config::CORTANA_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::CORTANA_APPEARANCE = { swca::ACCENT::ACCENT_NORMAL, 0x0 };

// Timeline/Task View
bool Config::TIMELINE_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::TIMELINE_APPEARANCE = { swca::ACCENT::ACCENT_NORMAL, 0x0 };

// Peek
enum Config::PEEK Config::PEEK = PEEK::Dynamic;
bool Config::PEEK_ONLY_MAIN = true;

// Advanced
uint8_t Config::SLEEP_TIME = 10;
bool Config::NO_TRAY = false;
bool Config::VERBOSE =
#ifndef _DEBUG
	false;
#else
	true;
#endif

std::mutex Config::m_ConfigLock;

const std::wstring Config::CLI_HELP_MSG = LR"(
	Hello world!
)";

const std::pair<const std::wstring, bool &> Config::CLI_FLAGS[] = {
	{ L"--dynamic-ws", MAXIMISED_ENABLED },
	{ L"--dynamic-ws-regular-on-peek", MAXIMISED_REGULAR_ON_PEEK },
	{ L"--dynamic-start", START_ENABLED },
	{ L"--dynamic-cortana", CORTANA_ENABLED },
	{ L"--dynamic-timeline", TIMELINE_ENABLED },
	{ L"--peek-only-main", PEEK_ONLY_MAIN },
	{ L"--no-tray", NO_TRAY },
	{ L"--verbose", VERBOSE }
};

void Config::Parse(const std::wstring &file)
{
	std::lock_guard guard(m_ConfigLock);

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

		size_t split_index = line.find(L'=');
		if (split_index != std::wstring::npos)
		{
			Util::ToLowerInplace(line);
			const std::wstring key = Util::Trim(line.substr(0, split_index));
			const std::wstring val = Util::Trim(line.substr(split_index + 1, line.length() - split_index - 1));

			ParseSingleConfigOption(key, val, Log::OutputMessage);
		}
		else
		{
			Log::OutputMessage(L"Invalid line in configuration file: " + line);
		}
	}
}

bool Config::ParseCommandLine()
{
	std::lock_guard guard(m_ConfigLock);

	std::vector<std::wstring> args = GetArgs();
	if (args.empty())
	{
		return true;
	}

	std::for_each(args.begin(), args.end(), Util::ToLowerInplace);
	std::for_each(args.begin(), args.end(), std::bind(&Util::TrimInplace, std::placeholders::_1, L' '));

	if (std::find(args.begin(), args.end(), L"--help") != args.end())
	{
		MessageBox(Window::NullWindow, CLI_HELP_MSG.c_str(), NAME, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
		return false;
	}
	else
	{
		std::wostringstream output;
		auto logger = [&output](const std::wstring &text)
		{
			output << text << std::endl;
		};

		if (args.size() > 0)
		{
			// note: std::vector::erase's last item is non-inclusive, hence the use of + 2 over + 1

			for (const auto &pair : CLI_FLAGS)
			{
				auto iter = std::find(args.begin(), args.end(), pair.first);
				if (iter != args.end())
				{
					if (iter + 1 != args.end() && !Util::StringBeginsWith(*(iter + 1), L"--"))
					{
						if (!ParseBool(*(iter + 1), pair.second))
						{
							UnknownValue(Util::RemovePrefix(*iter, L"--"), *(iter + 1), logger);
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

			while (args.size() >= 2)
			{
				Util::RemovePrefixInplace(args[0], L"--");

				ParseSingleConfigOption(args[0], args[1], logger);
				args.erase(args.begin(), args.begin() + 2);
			}

			if (args.size() != 0)
			{
				output << L"Orphaned CLI option detected: " << args[0] << std::endl;
			}
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
	std::lock_guard guard(m_ConfigLock);

	std::wofstream configstream(file);

	configstream << L"accent=" << std::left << std::setw(6) << std::setfill(L' ') << GetAccentText(REGULAR_APPEARANCE.ACCENT) << L"; accent values are: clear (default), fluent (only on build " << MIN_FLUENT_BUILD << L" and up), opaque, normal, or blur." << std::endl;
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
	case PEEK::Dynamic:
		configstream << L"dynamic";
		break;
	case PEEK::Enabled:
		configstream << L"show";
		break;
	}
	configstream << std::endl;
	configstream << L"peek-only-main=" << GetBoolText(PEEK_ONLY_MAIN) << L" ; Decides wether only the main monitor is considered when dynamic peek is enabled." << std::endl;

	configstream << std::endl;
	configstream << L"; Advanced settings" << std::endl;
	configstream << L"; sleep time in milliseconds, a shorter time reduces flicker when opening start, but results in higher CPU usage." << std::endl;
	configstream << L"sleep-time=" << std::dec << SLEEP_TIME << std::endl;
	configstream << L"; hide icon in system tray. Changes to this requires a restart of the application." << std::endl;
	configstream << L"no-tray=" << GetBoolText(NO_TRAY) << std::endl;
	configstream << L"; more informative logging. Can make huge log files." << std::endl;
	configstream << L"verbose=" << GetBoolText(VERBOSE) << std::endl;
}

std::vector<std::wstring> Config::GetArgs()
{
	int argc;
	const AutoFree::Local<wchar_t *> argv = CommandLineToArgvW(GetCommandLine(), &argc);
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

bool Config::ParseColor(std::wstring value, uint32_t &color)
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

bool Config::ParseOpacity(const std::wstring &value, uint32_t &color)
{
	try
	{
		color = ((std::stoi(value) & 0xFF) << 24) + (color & 0x00FFFFFF);
		return true;
	}
	catch (...)
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

void Config::ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value, const std::function<void(const std::wstring &)> &logger)
{
	if (arg == L"accent")
	{
		if (!ParseAccent(value, REGULAR_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"color" || arg == L"tint")
	{
		if (!ParseColor(value, REGULAR_APPEARANCE.COLOR))
		{
			logger(L"Could not parse color: " + value);
		}
	}
	else if (arg == L"opacity")
	{
		if (!ParseOpacity(value, REGULAR_APPEARANCE.COLOR))
		{
			logger(L"Could not parse opacity: " + value);
		}
	}
	else if (arg == L"dynamic-ws")
	{
		if (!ParseBool(value, MAXIMISED_ENABLED))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-ws-accent")
	{
		if (!ParseAccent(value, MAXIMISED_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-ws-color" || arg == L"dynamic-ws-tint")
	{
		if (!ParseColor(value, MAXIMISED_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic windows color: " + value);
		}
	}
	else if (arg == L"dynamic-ws-opacity")
	{
		if (!ParseOpacity(value, MAXIMISED_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic windows opacity: " + value);
		}
	}
	else if (arg == L"dynamic-ws-regular-on-peek")
	{
		if (!ParseBool(value, MAXIMISED_REGULAR_ON_PEEK))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-start")
	{
		if (!ParseBool(value, START_ENABLED))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-start-accent")
	{
		if (!ParseAccent(value, START_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-start-color" || arg == L"dynamic-start-tint")
	{
		if (!ParseColor(value, START_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic start color: " + value);
		}
	}
	else if (arg == L"dynamic-start-opacity")
	{
		if (!ParseOpacity(value, START_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic start opacity: " + value);
		}
	}
	else if (arg == L"dynamic-cortana")
	{
		if (!ParseBool(value, CORTANA_ENABLED))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-cortana-accent")
	{
		if (!ParseAccent(value, CORTANA_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-cortana-color" || arg == L"dynamic-cortana-tint")
	{
		if (!ParseColor(value, CORTANA_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic Cortana color: " + value);
		}
	}
	else if (arg == L"dynamic-cortana-opacity")
	{
		if (!ParseOpacity(value, CORTANA_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic Cortana opacity: " + value);
		}
	}
	else if (arg == L"dynamic-timeline")
	{
		if (!ParseBool(value, TIMELINE_ENABLED))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-timeline-accent")
	{
		if (!ParseAccent(value, TIMELINE_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"dynamic-timeline-color" || arg == L"dynamic-timeline-tint")
	{
		if (!ParseColor(value, TIMELINE_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic timeline color: " + value);
		}
	}
	else if (arg == L"dynamic-timeline-opacity")
	{
		if (!ParseOpacity(value, TIMELINE_APPEARANCE.COLOR))
		{
			logger(L"Could not parse dynamic timeline opacity: " + value);
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
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"peek-only-main")
	{
		if (!ParseBool(value, PEEK_ONLY_MAIN))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"sleep-time")
	{
		try
		{
			SLEEP_TIME = std::stoi(value) & 0xFF;
		}
		catch (...)
		{
			logger(L"Could not parse sleep time: " + value);
		}
	}
	else if (arg == L"no-tray")
	{
		if (!ParseBool(value, NO_TRAY))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else if (arg == L"verbose")
	{
		if (!ParseBool(value, VERBOSE))
		{
			UnknownValue(arg, value, logger);
		}
	}
	else
	{
		logger(L"Unknown key found: " + arg);
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