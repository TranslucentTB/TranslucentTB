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

	static std::time_t GetProcessCreationTime() noexcept;
	static std::filesystem::path GetPath();
	static void LogErrorHandler(const std::string &message);
	static void Initialize();

#ifdef PROGRAMLOG_EXPORTS
	friend BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) noexcept;
#endif

public:
	inline static std::shared_ptr<lazy_file_sink_mt> GetSink()
	{
		return s_LogSink.lock();
	}
};
