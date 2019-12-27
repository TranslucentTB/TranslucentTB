#include "error.hpp"

#include <spdlog/spdlog.h>
#include <thread>

#include "window.hpp"

void Error::impl::Log(std::wstring_view msg, spdlog::level::level_enum level, const char* file, int line, const char* function)
{
	spdlog::log({ file, line, function }, level, msg);
}

std::wstring Error::impl::GetLogMessage(std::wstring_view message, std::wstring_view error_message, std::wstring_view err_message_fmt, std::wstring_view message_fmt)
{
	if (!error_message.empty())
	{
		return fmt::format(err_message_fmt, message, error_message);
	}
	else
	{
		return fmt::format(message_fmt, message);
	}
}

template<>
void Error::Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, const char* file, int line, const char* function)
{
	impl::Log(impl::GetLogMessage(message, error_message), spdlog::level::err, file, line, function);

	std::thread([error = impl::GetLogMessage(message, error_message, ERROR_MESSAGE L"\n\n{}\n\n{}", ERROR_MESSAGE L"\n\n{}")]
	{
		MessageBoxEx(Window::NullWindow, error.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
	}).detach();
}

template<>
void Error::Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, const char* file, int line, const char* function)
{
	impl::Log(impl::GetLogMessage(message, error_message), spdlog::level::critical, file, line, function);

	const std::wstring msg = impl::GetLogMessage(message, error_message, FATAL_ERROR_MESSAGE L"\n\n{}\n\n{}", FATAL_ERROR_MESSAGE L"\n\n{}");
	// TODO: this allows normal message loop to run, potentially bad. consider kick off a thread and block.
	MessageBoxEx(Window::NullWindow, msg.c_str(), FATAL_ERROR_TITLE, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));

	// Calling abort() will generate a dialog box, but we already have our own.
	// Raising a fail-fast exception skips it but also allows WER to do its job.
	RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
}
