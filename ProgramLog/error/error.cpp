#include "arch.h"
#include "error.hpp"

#include <spdlog/spdlog.h>
#include <string>
#include <processthreadsapi.h>
#include <wil/resource.h>

#include "win32.hpp"
#include "window.hpp"

void Error::impl::Log(const fmt::wmemory_buffer &msg, spdlog::level::level_enum level, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function)
{
	spdlog::log({ file.c_str(), line, function.c_str() }, level, Util::ToStringView(msg));
}

void Error::impl::GetLogMessage(fmt::wmemory_buffer &out, std::wstring_view message, std::wstring_view error_message, std::wstring_view err_message_fmt, std::wstring_view message_fmt)
{
	if (!error_message.empty())
	{
		fmt::format_to(out, err_message_fmt, message, error_message);
	}
	else
	{
		fmt::format_to(out, message_fmt, message);
	}
}

template<>
void Error::impl::Handle<spdlog::level::err>(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function)
{
	fmt::wmemory_buffer buf;
	GetLogMessage(buf, message, error_message);
	Log(buf, spdlog::level::err, file, line, function);

	buf.clear();
	GetLogMessage(buf, message, error_message, ERROR_MESSAGE L"\n\n{}\n\n{}", ERROR_MESSAGE L"\n\n{}");

	CreateMessageBoxThread(buf, ERROR_TITLE, MB_ICONWARNING);
}

template<>
void Error::impl::Handle<spdlog::level::critical>(std::wstring_view message, std::wstring_view error_message, Util::null_terminated_string_view file, int line, Util::null_terminated_string_view function)
{
	fmt::wmemory_buffer buf;
	GetLogMessage(buf, message, error_message);
	Log(buf, spdlog::level::critical, file, line, function);

	buf.clear();
	GetLogMessage(buf, message, error_message, FATAL_ERROR_MESSAGE L"\n\n{}\n\n{}", FATAL_ERROR_MESSAGE L"\n\n{}");

	auto thread = CreateMessageBoxThread(buf, FATAL_ERROR_TITLE, MB_ICONERROR | MB_TOPMOST);

	if (thread)
	{
		if (WaitForSingleObject(thread.get(), INFINITE) == WAIT_FAILED)
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to wait for thread");
		}
	}

	// Calling abort() will generate a dialog box, but we already have our own.
	// Raising a fail-fast exception skips it but also allows WER to do its job.
	RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
	__assume(0);
}

wil::unique_handle Error::CreateMessageBoxThread(const fmt::wmemory_buffer &buf, Util::null_terminated_wstring_view title, unsigned int type)
{
	struct thread_info {
		std::wstring body;
		Util::null_terminated_wstring_view title;
		unsigned int type;
	};

	auto info = std::make_unique<thread_info>();
	info->title = title;
	info->type = type;

	wil::unique_handle thread(CreateThread(nullptr, 0, [](void *userData) -> DWORD
	{
		std::unique_ptr<thread_info> info(static_cast<thread_info *>(userData));

		MessageBoxEx(Window::NullWindow, info->body.c_str(), info->title.c_str(), info->type | MB_OK | MB_SETFOREGROUND, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));

		return 0;
	}, info.get(), CREATE_SUSPENDED, nullptr));

	if (thread)
	{
		// Allocate only if thread is created
		info->body = fmt::to_string(buf);

		if (ResumeThread(thread.get()) != -1)
		{
			// Ownership transferred to thread
			static_cast<void>(info.release());
		}
		else
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to resume thread");
		}
	}
	else
	{
		LastErrorHandle(spdlog::level::warn, L"Failed to create thread");
	}

	// Transfer ownership to caller
	return thread;
}
