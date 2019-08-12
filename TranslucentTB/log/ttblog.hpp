#pragma once
#include "arch.h"
#include <ctime>
#include <filesystem>
#include <memory>
#include <spdlog/common.h>
#include <string>
#include <windef.h>

#include "lazyfilesink.hpp"
#include "win32.hpp"

class Log {
private:
	static std::weak_ptr<lazy_file_sink_mt> s_LogSink;
	static bool s_InitDone;

	static std::time_t GetProcessCreationTime();
	static std::filesystem::path GetPath();
	static void HandleInitializationError(std::wstring exception);
	static void LogErrorHandler(const std::string &message);

public:
	enum InitializationState {
		Done,
		Failed,
		NotInitialized
	};

	static void Initialize();

	inline static spdlog::level::level_enum GetLevel()
	{
		if (s_InitDone)
		{
			if (const auto sink = s_LogSink.lock(); sink && !sink->error())
			{
				return sink->level();
			}
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

	inline static InitializationState GetInitializationState()
	{
		if (const auto sink = s_LogSink.lock())
		{
			if (sink->opened())
			{
				return Done;
			}
			else if (sink->error())
			{
				return Failed;
			}
		}
		else if (s_InitDone)
		{
			return Failed;
		}

		return NotInitialized;
	}

	inline static void Open()
	{
		if (const auto sink = s_LogSink.lock(); sink && sink->opened())
		{
			sink->flush();
			win32::EditFile(sink->file());
		}
	}
};
