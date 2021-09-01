#pragma once
#include <atomic>
#include <ctime>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "api.h"
#include "lazyfilesink.hpp"

class Log {
private:
	enum class InitStatus {
		NotInitialized,
		Initializing,
		Initialized
	};

	static std::atomic<InitStatus> s_LogInitStatus;
	static std::weak_ptr<lazy_file_sink_mt> s_LogSink;

	static std::time_t GetProcessCreationTime() noexcept;
	static std::filesystem::path GetPath(const std::optional<std::filesystem::path> &storageFolder);
	static void LogErrorHandler(const std::string &message);

public:
	PROGRAMLOG_API static bool IsInitialized() noexcept;
	PROGRAMLOG_API static std::shared_ptr<lazy_file_sink_mt> GetSink() noexcept;
	PROGRAMLOG_API static void Initialize(const std::optional<std::filesystem::path> &storageFolder);
};
