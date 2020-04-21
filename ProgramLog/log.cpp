#include "log.hpp"
#include <chrono>
#include <debugapi.h>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/msvc_sink.h>
#include <wil/resource.h>
#include "winrt.hpp"
#include <winrt/Windows.Storage.h>

#include "appinfo.hpp"
#include "config/config.hpp"
#include "uwp.hpp"

std::weak_ptr<lazy_file_sink_st> Log::s_LogSink;

std::time_t Log::GetProcessCreationTime() noexcept
{
	if (FILETIME creationTime, exitTime, kernelTime, userTime; GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime))
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

std::filesystem::path Log::GetPath()
{
	std::filesystem::path path;
	if (UWP::HasPackageIdentity())
	{
		using namespace winrt::Windows::Storage;
		path = std::wstring_view(ApplicationData::Current().TemporaryFolder().Path());
	}
	else
	{
		path = std::filesystem::temp_directory_path();
	}

	return path / fmt::format(fmt(L"{}.log"), GetProcessCreationTime());
}

void Log::LogErrorHandler(const std::string &message)
{
	fmt::memory_buffer buf;
	fmt::format_to(buf, fmt("An error has been encountered while logging a message.\n\n{}"), message);
	buf.push_back('\0');
	MessageBoxExA(Window::NullWindow, buf.data(), UTF8_ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
}

void Log::Initialize()
{
	using namespace spdlog;

	set_formatter(std::make_unique<pattern_formatter>(pattern_time_type::utc));
	set_level(level::trace);
	flush_on(level::off);
	set_error_handler(LogErrorHandler);

	auto defaultLogger = std::make_shared<logger>("");
	initialize_logger(defaultLogger);

	if (IsDebuggerPresent())
	{
		defaultLogger->sinks().push_back(std::make_shared<sinks::windebug_sink_st>());
	}

	const auto fileLog = std::make_shared<lazy_file_sink_st>(GetPath);
	fileLog->set_level(Config { }.LogVerbosity);
	defaultLogger->sinks().push_back(fileLog);

	set_default_logger(std::move(defaultLogger));

	s_LogSink = fileLog;
}
