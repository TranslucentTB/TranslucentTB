#include "log.hpp"
#include <chrono>
#include <debugapi.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/logger.h>
#include "winrt.hpp"

#include "appinfo.hpp"
#include "config/config.hpp"
#include "error/error.hpp"
#include "error/winrt.hpp"
#include "error/std.hpp"

std::atomic<Log::InitStatus> Log::s_LogInitStatus = Log::InitStatus::NotInitialized;
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
		if (!code)
		{
			path /= APP_NAME;
		}
		else
		{
			StdErrorCodeHandle(code, spdlog::level::err, L"Failed to get log file path. Logs won't be available during this session");

			return { };
		}
	}

	path /= std::format(L"{}.log", GetProcessCreationTime());
	return path;
}

void Log::LogErrorHandler(const std::string &message)
{
	static constexpr std::string_view BODY_BASE = "An error has been encountered while logging a message.\n\n";

	std::string dialogMessage;
	dialogMessage.reserve(BODY_BASE.length() + message.length());
	dialogMessage += BODY_BASE;
	dialogMessage += message;

	MessageBoxExA(nullptr, dialogMessage.c_str(), UTF8_ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
}

bool Log::IsInitialized() noexcept
{
	return s_LogInitStatus == InitStatus::Initialized;
}

std::shared_ptr<lazy_file_sink_mt> Log::GetSink() noexcept
{
	if (IsInitialized())
	{
		return s_LogSink.lock();
	}
	else
	{
		return nullptr;
	}
}

void Log::Initialize(const std::optional<std::filesystem::path> &storageFolder)
{
	auto expected = InitStatus::NotInitialized;
	if (s_LogInitStatus.compare_exchange_strong(expected, InitStatus::Initializing))
	{
		auto defaultLogger = std::make_shared<spdlog::logger>("", std::make_shared<spdlog::sinks::windebug_sink_st>());

		if (auto path = GetPath(storageFolder); !path.empty())
		{
			auto fileLog = std::make_shared<lazy_file_sink_mt>(std::move(path));
			fileLog->set_level(Config::DEFAULT_LOG_VERBOSITY);
			s_LogSink = fileLog;

			defaultLogger->sinks().push_back(std::move(fileLog));
		}

		spdlog::set_formatter(std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::utc));
		spdlog::set_level(spdlog::level::trace);
		spdlog::flush_on(spdlog::level::off);
		spdlog::set_error_handler(LogErrorHandler);
		spdlog::initialize_logger(defaultLogger);
		spdlog::set_default_logger(std::move(defaultLogger));

		s_LogInitStatus = InitStatus::Initialized;
	}
	else
	{
		throw std::logic_error("Log::Initialize should only be called once");
	}
}
