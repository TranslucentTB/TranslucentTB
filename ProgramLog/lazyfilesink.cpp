#include "lazyfilesink.hpp"
#include <fileapi.h>
#include <fmt/format.h>
#include <utility>
#include <wil/safecast.h>
#include "winrt.hpp"

#include "appinfo.hpp"
#include "constants.hpp"
#include "window.hpp"
#include "error/error.hpp"
#include "error/std.hpp"
#include "error/win32.hpp"
#include "error/winrt.hpp"
#include "util/to_string_view.hpp"

template<typename Mutex>
void lazy_file_sink<Mutex>::sink_it_(const spdlog::details::log_msg &msg)
{
	open();

	if (m_Handle)
	{
		spdlog::memory_buf_t formatted;
		this->formatter_->format(msg, formatted);

		write(formatted);
	}
}

template<typename Mutex>
void lazy_file_sink<Mutex>::flush_()
{
	if (m_Handle)
	{
		if (!FlushFileBuffers(m_Handle.get()))
		{
			LastErrorHandle(spdlog::level::trace, L"Failed to flush log file.");
		}
	}
}

template<typename Mutex>
void lazy_file_sink<Mutex>::open()
{
	if (!std::exchange(m_Tried, true))
	{
		m_Handle.reset(CreateFile(m_File.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
		if (m_Handle)
		{
			write(UTF8_BOM);
		}
		else
		{
			LastErrorHandle(spdlog::level::err, L"Failed to create log file. Logs won't be available during this session.");
		}
	}
}

template<typename Mutex>
template<typename T>
void lazy_file_sink<Mutex>::write(const T &thing)
{
	DWORD bytesWritten;
	if (!WriteFile(m_Handle.get(), thing.data(), wil::safe_cast<DWORD>(thing.size()), &bytesWritten, nullptr))
	{
		LastErrorHandle(spdlog::level::trace, L"Failed to write log entry to file.");
		return;
	}

	if (bytesWritten != thing.size())
	{
		MessagePrint(spdlog::level::trace, L"Wrote less characters than there is in log entry.");
	}
}

// We don't actually use the singlethreaded sink.
template class lazy_file_sink<std::mutex>;
