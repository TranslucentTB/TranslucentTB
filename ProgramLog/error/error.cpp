#include "error.hpp"
#include <debugapi.h>
#include <intrin.h>
#include <spdlog/spdlog.h>
#include <string>
#ifdef _DEBUG
#include <processthreadsapi.h>
#endif
#include <wil/resource.h>
#include <winnt.h>

#include "../log.hpp"
#include "std.hpp"
#include "util/string_macros.hpp"
#include "win32.hpp"

bool Error::impl::ShouldLogInternal(spdlog::level::level_enum level)
{
	// implicitly checks if logging is initialized.
	if (const auto sink = Log::GetSink())
	{
		return sink->should_log(level) || IsDebuggerPresent();
	}

	return false;
}

void Error::impl::Log(std::wstring_view msg, spdlog::level::level_enum level, std::source_location location)
{
	spdlog::log({ location.file_name(), static_cast<int>(location.line()), location.function_name() }, level, msg);
}

std::wstring Error::impl::GetLogMessage(std::wstring_view message, std::wstring_view error_message)
{
	if (!error_message.empty())
	{
		std::wstring logMessage;
		logMessage.reserve(message.length() + 2 + error_message.length() + 1);
		logMessage += message;
		logMessage += L" (";
		logMessage += error_message;
		logMessage += L")";

		return logMessage;
	}
	else
	{
		return std::wstring(message);
	}
}

template<>
void Error::impl::Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, std::source_location location)
{
	auto dialogBoxThread = HandleCommon(spdlog::level::err, message, error_message, location, UTIL_WIDEN(UTF8_ERROR_TITLE), APP_NAME L" has encountered an error.", MB_ICONWARNING);
	if (dialogBoxThread.joinable())
	{
		dialogBoxThread.detach();
	}
	else
	{
		DebugBreak();
	}
}

template<>
void Error::impl::Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, std::source_location location)
{
	HandleCriticalCommon(message, error_message, location);
	__fastfail(FAST_FAIL_FATAL_APP_EXIT);
}

std::thread Error::impl::HandleCommon(spdlog::level::level_enum level, std::wstring_view message, std::wstring_view error_message, std::source_location location, Util::null_terminated_wstring_view title, std::wstring_view description, unsigned int type)
{
	// allow calls to err handling without needing to initialize logging
	if (Log::IsInitialized())
	{
		Log(GetLogMessage(message, error_message), level, location);
	}

	if (!IsDebuggerPresent())
	{
		std::size_t messageLength = description.length() + 2 + message.length();
		const bool hasErrorMessage = !error_message.empty();
		if (hasErrorMessage)
		{
			messageLength += 2;
			messageLength += error_message.length();
		}

		std::wstring dialogMessage;
		dialogMessage.reserve(messageLength);
		dialogMessage += description;
		dialogMessage += L"\n\n";
		dialogMessage += message;

		if (hasErrorMessage)
		{
			dialogMessage += L"\n\n";
			dialogMessage += error_message;
		}

		return std::thread([title, type, body = std::move(dialogMessage)]() noexcept
		{
			MessageBoxEx(nullptr, body.c_str(), title.c_str(), type | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
		});
	}
	else
	{
		return {};
	}
}

void Error::impl::HandleCriticalCommon(std::wstring_view message, std::wstring_view error_message, std::source_location location)
{
	auto dialogBoxThread = HandleCommon(spdlog::level::critical, message, error_message, location, UTIL_WIDEN(UTF8_ERROR_TITLE), APP_NAME L" has encountered a fatal error and cannot continue executing.", MB_ICONERROR | MB_TOPMOST);
	if (dialogBoxThread.joinable())
	{
		dialogBoxThread.join();
	}
}
