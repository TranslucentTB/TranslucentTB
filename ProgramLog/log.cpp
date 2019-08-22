#include "log.hpp"
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/msvc_sink.h>
#include <winrt/base.h>

#include "appinfo.hpp"
#include "config/config.hpp"
//#include "../uwp/uwp.hpp"

std::weak_ptr<lazy_file_sink_mt> Log::s_LogSink;

std::time_t Log::GetProcessCreationTime()
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
	std::filesystem::path name;
	/*if (UWP::HasPackageIdentity())
	{
		name = static_cast<std::wstring_view>(UWP::GetApplicationFolderPath(UWP::FolderType::Temporary));
	}
	else*/
	{
		name = std::filesystem::temp_directory_path();
	}

	return name / fmt::format(fmt(L"{}.log"), GetProcessCreationTime());
}

void Log::HandleInitializationError(std::wstring exception)
{
	std::wstring message = L"Failed to determine log file path. Logs won't be available during this session.\n\n" + std::move(exception);

	std::thread([msg = std::move(message)]()
	{
		MessageBox(Window::NullWindow, msg.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
	}).detach();
}

void Log::LogErrorHandler(const std::string &message)
{
	const std::string err = fmt::format(fmt("An error has been encountered while logging a message.\n\n{}"), message);
	MessageBoxA(Window::NullWindow, err.c_str(), UTF8_ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
}

void Log::Initialize()
{
	spdlog::flush_on(spdlog::level::off);
	spdlog::set_error_handler(LogErrorHandler);

	auto logger = std::make_shared<spdlog::logger>("");
	spdlog::initialize_logger(logger);

	logger->sinks().push_back(std::make_shared<spdlog::sinks::windebug_sink_st>());

	std::shared_ptr<lazy_file_sink_mt> file_log;
	try
	{
		file_log = std::make_shared<lazy_file_sink_mt>(GetPath());

		// default verbosity
		file_log->set_level(Config{ }.LogVerbosity);

		logger->sinks().push_back(file_log);
	}
	catch (const std::system_error &err)
	{
		HandleInitializationError(Error::MessageFromHRESULT(err.code().value()));
	}
	catch (const winrt::hresult_error &err)
	{
		HandleInitializationError(Error::MessageFromHresultError(err));
	}

	spdlog::set_default_logger(std::move(logger));

	s_LogSink = file_log;
}
