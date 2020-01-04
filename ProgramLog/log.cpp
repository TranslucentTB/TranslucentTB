#include "log.hpp"
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/msvc_sink.h>
#include <wil/resource.h>
#include <winrt/Windows.Storage.h>

#include "appinfo.hpp"
#include "config/config.hpp"
#include "uwp.hpp"

std::weak_ptr<lazy_file_sink_mt> Log::s_LogSink;

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
	const std::string err = fmt::format(fmt("An error has been encountered while logging a message.\n\n{}"), message);
	MessageBoxExA(Window::NullWindow, err.c_str(), UTF8_ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
}

void Log::Initialize()
{
	spdlog::flush_on(spdlog::level::off);
	spdlog::set_error_handler(LogErrorHandler);

	auto logger = std::make_shared<spdlog::logger>("");
	spdlog::initialize_logger(logger);

	logger->sinks().push_back(std::make_shared<spdlog::sinks::windebug_sink_st>());

	const auto file_log = std::make_shared<lazy_file_sink_mt>(GetPath);
	file_log->set_level(Config { }.LogVerbosity);
	logger->sinks().push_back(file_log);

	spdlog::set_default_logger(std::move(logger));

	s_LogSink = file_log;
}
