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
#include "error/error.hpp"
#include "error/winrt.hpp"
#include "error/std.hpp"

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

std::filesystem::path Log::GetPath(bool hasPackageIdentity)
{
	static constexpr std::wstring_view FAILED_TO_GET_PATH = L"Failed to get log file path.";

	std::filesystem::path path;
	fmt::wmemory_buffer buf;
	if (hasPackageIdentity)
	{
		try
		{
			path = std::wstring_view(winrt::Windows::Storage::ApplicationData::Current().TemporaryFolder().Path());
		}
		catch (const winrt::hresult_error &err)
		{
			HresultErrorHandleWithBuffer(buf, err, spdlog::level::trace, FAILED_TO_GET_PATH);
			HandlePathError(Util::ToStringView(buf));

			return { };
		}
	}
	else
	{
		std::error_code code;
		path = std::filesystem::temp_directory_path(code);
		if (code)
		{
			StdErrorCodeHandleWithBuffer(buf, code, spdlog::level::trace, FAILED_TO_GET_PATH);
			HandlePathError(Util::ToStringView(buf));

			return { };
		}
	}

	fmt::format_to(buf, FMT_STRING(L"{}.log"), GetProcessCreationTime());
	return path / Util::ToStringView(buf);
}

void Log::HandlePathError(std::wstring_view err)
{
	fmt::wmemory_buffer buf;
	fmt::format_to(buf, FMT_STRING(L"Failed to get log file path. Logs won't be available during this session.\n\n{}"), err);
	Error::impl::CreateMessageBoxThread(buf, ERROR_TITLE, MB_ICONWARNING).detach();
}

void Log::LogErrorHandler(const std::string &message)
{
	fmt::memory_buffer buf;
	fmt::format_to(buf, FMT_STRING("An error has been encountered while logging a message.\n\n{}"), message);
	buf.push_back('\0');
	MessageBoxExA(nullptr, buf.data(), UTF8_ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
}

void Log::Initialize(bool hasPackageIdentity)
{
	spdlog::set_formatter(std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::utc));
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::off);
	spdlog::set_error_handler(LogErrorHandler);

	auto defaultLogger = std::make_shared<spdlog::logger>("");
	spdlog::initialize_logger(defaultLogger);

	if (IsDebuggerPresent())
	{
		// always single-threaded because OutputDebugString is already thread-safe
		defaultLogger->sinks().push_back(std::make_shared<spdlog::sinks::windebug_sink_st>());
	}

	spdlog::set_default_logger(defaultLogger);

	if (auto path = GetPath(hasPackageIdentity); !path.empty())
	{
		const auto fileLog = std::make_shared<lazy_file_sink_mt>(std::move(path));
		fileLog->set_level(Config{ }.LogVerbosity);
		defaultLogger->sinks().push_back(fileLog);

		s_LogSink = fileLog;
	}
}
