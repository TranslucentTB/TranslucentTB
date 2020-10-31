#pragma once
#include <ctime>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "api.h"
#include "lazyfilesink.hpp"

class Log {
private:
	PROGRAMLOG_API static std::weak_ptr<lazy_file_sink_mt> s_LogSink;

	static std::time_t GetProcessCreationTime() noexcept;
	static std::filesystem::path GetPath(bool hasPackageIdentity);
	static void HandlePathError(std::wstring_view err);
	static void LogErrorHandler(const std::string &message);

public:
	inline static std::shared_ptr<lazy_file_sink_mt> GetSink() noexcept
	{
		return s_LogSink.lock();
	}

	PROGRAMLOG_API static void Initialize(bool hasPackageIdentity);
};
