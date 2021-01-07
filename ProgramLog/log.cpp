#include "log.hpp"
#include <chrono>
#include <debugapi.h>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include "winrt.hpp"

#include "appinfo.hpp"
#include "config/config.hpp"
#include "debugsink.hpp"
#include "error/error.hpp"
#include "error/winrt.hpp"
#include "error/std.hpp"
#include "util/fmt.hpp"
#include "util/to_string_view.hpp"

bool Log::s_LogInitialized = false;
std::weak_ptr<lazy_file_sink_mt> Log::s_LogSink;

std::time_t Log::GetProcessCreationTime() noexcept
{
	FILETIME creationTime, exitTime, kernelTime, userTime;
	if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime))
	{
		using winrt::clock;
		return clock::to_time_t(clock::from_file_time(creationTime));
	}
	else
	{
		// Fallback to current time
		using std::chrono::system_clock;
		return system_clock::to_time_t(system_clock::now());
	}
}

std::filesystem::path Log::GetPath(const std::optional<std::filesystem::path> &storageFolder)
{
	std::filesystem::path path;
	if (storageFolder)
	{
		path = *storageFolder / L"TempState";
	}
	else
	{
		std::error_code code;
		path = std::filesystem::temp_directory_path(code);
		if (code) [[unlikely]]
		{
			StdErrorCodeHandle(code, spdlog::level::err, L"Failed to get log file path. Logs won't be available during this session");

			return { };
		}
	}

	Util::small_wmemory_buffer<25> buf;
	fmt::format_to(buf, FMT_STRING(L"{}.log"), GetProcessCreationTime());
	path /= Util::ToStringView(buf);
	return path;
}

void Log::LogErrorHandler(const std::string &message)
{
	fmt::memory_buffer buf;
	fmt::format_to(buf, FMT_STRING("An error has been encountered while logging a message.\n\n{}"), message);
	buf.push_back('\0');
	MessageBoxExA(nullptr, buf.data(), UTF8_ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
}

void Log::Initialize(const std::optional<std::filesystem::path> &storageFolder)
{
	auto defaultLogger = std::make_shared<spdlog::logger>("", std::make_shared<debug_sink>());

	if (auto path = GetPath(storageFolder); !path.empty())
	{
		auto fileLog = std::make_shared<lazy_file_sink_mt>(std::move(path));
		fileLog->set_level(Config{ }.LogVerbosity);
		s_LogSink = fileLog;

		defaultLogger->sinks().push_back(std::move(fileLog));
	}

	spdlog::set_formatter(std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::utc));
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::off);
	spdlog::set_error_handler(LogErrorHandler);
	spdlog::initialize_logger(defaultLogger);
	spdlog::set_default_logger(std::move(defaultLogger));

	s_LogInitialized = true;
}
