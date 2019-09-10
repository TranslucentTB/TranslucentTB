#pragma once
#include "arch.h"
#include <ctime>
#include <filesystem>
#include <memory>
#include <spdlog/common.h>
#include <string>
#include <windef.h>

#include "api.h"
#include "lazyfilesink.hpp"
#include "win32.hpp"

class Log {
private:
	PROGRAMLOG_API static std::weak_ptr<lazy_file_sink_mt> s_LogSink;

	static std::time_t GetProcessCreationTime();
	static std::filesystem::path GetPath();
	static void LogErrorHandler(const std::string &message);
	static void Initialize();

#ifdef PROGRAMLOG_EXPORTS
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
#endif

public:

	inline static spdlog::level::level_enum GetLevel()
	{
		if (const auto sink = s_LogSink.lock(); sink && sink->opened())
		{
			return sink->level();
		}

		return spdlog::level::off;
	}

	inline static void SetLevel(spdlog::level::level_enum level)
	{
		if (const auto sink = s_LogSink.lock())
		{
			sink->set_level(level);
		}
	}

	enum Status {
		Ok,
		NothingLogged,
		FailedOpening,
		FailedInitializing
	};

	inline static Status GetStatus()
	{
		if (const auto sink = s_LogSink.lock())
		{
			return sink->opened()
				? Ok
				: sink->tried()
					? FailedOpening
					: NothingLogged;
		}

		return FailedInitializing;
	}

	inline static void ViewFile()
	{
		if (const auto sink = s_LogSink.lock(); sink && sink->opened())
		{
			sink->flush();
			// TODO: update
			win32::EditFile(sink->file());
		}
	}
};
