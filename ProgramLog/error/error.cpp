#include "error.hpp"
#include <debugapi.h>
#include <intrin.h>
#include <roerrorapi.h>
#include <spdlog/spdlog.h>
#include <string>
#include <processthreadsapi.h>
#include <wil/resource.h>
#include <winnt.h>

#include "../log.hpp"
#include "std.hpp"
#include "util/string_macros.hpp"
#include "win32.hpp"

bool Error::ShouldLog(spdlog::level::level_enum level)
{
	if (level == spdlog::level::err || level == spdlog::level::critical)
	{
		return true;
	}

	// implicitly checks if logging is initialized.
	if (const auto sink = Log::GetSink())
	{
		return sink->should_log(level) || IsDebuggerPresent();
	}

	return false;
}

void Error::impl::Log(std::wstring_view msg, spdlog::level::level_enum level, std::source_location location)
{
	spdlog::log({ location.file_name(), location.line(), location.function_name() }, level, msg);
}

std::wstring Error::impl::GetLogMessage(std::wstring_view message, std::wstring_view error_message)
{
	if (!error_message.empty())
	{
		return std::format(L"{} ({})", message, error_message);
	}
	else
	{
		return std::wstring(message);
	}
}

template<>
void Error::impl::Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, std::source_location location, HRESULT, IRestrictedErrorInfo*)
{
	// allow calls to err handling without needing to initialize logging
	if (Log::IsInitialized())
	{
		Log(GetLogMessage(message, error_message), spdlog::level::err, location);
	}

	if (!IsDebuggerPresent())
	{
		auto dialogMessage = std::format(APP_NAME L" has encountered an error.\n\n{}", message);
		if (!error_message.empty())
		{
			dialogMessage.reserve(dialogMessage.size() + 2 + error_message.length());
			dialogMessage += L"\n\n";
			dialogMessage += error_message;
		}

		CreateMessageBoxThread(std::move(dialogMessage), UTIL_WIDEN(UTF8_ERROR_TITLE), MB_ICONWARNING).detach();
	}
	else
	{
		DebugBreak();
	}
}

template<>
void Error::impl::Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, std::source_location location, HRESULT err, IRestrictedErrorInfo *errInfo)
{
	// allow calls to critical handling without needing to initialize logging
	const bool initialized = Log::IsInitialized();
	if (initialized)
	{
		Log(GetLogMessage(message, error_message), spdlog::level::critical, location);
	}

	if (!IsDebuggerPresent())
	{
		auto dialogMessage = std::format(APP_NAME L" has encountered a fatal error and cannot continue executing.\n\n{}", message);
		if (!error_message.empty())
		{
			dialogMessage.reserve(dialogMessage.size() + 2 + error_message.length());
			dialogMessage += L"\n\n";
			dialogMessage += error_message;
		}

		CreateMessageBoxThread(std::move(dialogMessage), APP_NAME L" - Fatal error", MB_ICONERROR | MB_TOPMOST).join();
	}

	if (errInfo)
	{
		if (const HRESULT hr = SetRestrictedErrorInfo(errInfo); SUCCEEDED(hr))
		{
			// This gives much better error reporting if the error came from a WinRT module:
			// the stack trace in the dump, debugger and telemetry is unaffected by our error handling,
			// giving us better insight into what went wrong.
			RoFailFastWithErrorContext(err);
		}
		else if (initialized)
		{
			HresultHandle(hr, spdlog::level::warn, L"Failed to set restricted error info");
		}
	}

	__fastfail(FAST_FAIL_FATAL_APP_EXIT);
}

std::thread Error::impl::CreateMessageBoxThread(std::wstring buf, Util::null_terminated_wstring_view title, unsigned int type)
{
	return std::thread([title, type, body = std::move(buf)]() noexcept
	{
#ifdef _DEBUG
		SetThreadDescription(GetCurrentThread(), APP_NAME L" Message Box Thread"); // ignore error
#endif
		MessageBoxEx(nullptr, body.c_str(), title.c_str(), type | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
	});
}
