#pragma once
#include "arch.h"
#include <fileapi.h>
#include <filesystem>
#include <fmt/format.h>
#include <mutex>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <string_view>
#include <type_traits>
#include <utility>
#include <wil/resource.h>
#include <wil/safecast.h>
#include <winrt/base.h>

#include "appinfo.hpp"
#include "constants.hpp"
#include "window.hpp"
#include "error/error.hpp"
#include "error/std.hpp"
#include "error/win32.hpp"
#include "util/to_string_view.hpp"

template<typename Mutex>
class lazy_file_sink final : public spdlog::sinks::base_sink<Mutex> {
	using path_getter_t = std::add_pointer_t<std::filesystem::path()>;
public:
	explicit lazy_file_sink(path_getter_t getter) : m_PathGetter(getter), m_Tried(false) { }

	const std::filesystem::path &file() const noexcept { return m_File; }
	bool opened() const noexcept { return m_Handle.is_valid(); }
	bool tried() const noexcept { return m_Tried; }

protected:
	void sink_it_(const spdlog::details::log_msg &msg) override
	{
		open();

		if (m_Handle)
		{
			spdlog::memory_buf_t formatted;
			this->formatter_->format(msg, formatted);

			write(formatted);
		}
	}

	void flush_() override
	{
		if (m_Handle)
		{
			if (!FlushFileBuffers(m_Handle.get()))
			{
				LastErrorHandle(spdlog::level::trace, L"Failed to flush log file.");
			}
		}
	}

private:
	path_getter_t m_PathGetter;
	bool m_Tried;
	wil::unique_hfile m_Handle;
	std::filesystem::path m_File;

	void open()
	{
		if (!std::exchange(m_Tried, true))
		{
			fmt::wmemory_buffer buf;

			try
			{
				m_File = m_PathGetter();
			}
			catch (const winrt::hresult_error &err)
			{
				HresultErrorHandle(err, spdlog::level::trace, L"Failed to get log file path.");

				Error::MessageFromHresultError(buf, err);
				handle_open_error(Util::ToStringView(buf));

				return;
			}
			catch (const std::filesystem::filesystem_error &err)
			{
				StdSystemErrorHandle(err, spdlog::level::trace, L"Failed to get log file path.");

				Error::MessageFromStdSystemError(buf, err);
				handle_open_error(Util::ToStringView(buf));

				return;
			}

			m_Handle.reset(CreateFile(m_File.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
			if (m_Handle)
			{
				write(UTF8_BOM);
			}
			else
			{
				const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
				HresultHandle(hr, spdlog::level::trace, L"Failed to create log file.");

				Error::MessageFromHRESULT(buf, hr);
				handle_open_error(Util::ToStringView(buf));
			}
		}
	}

	void handle_open_error(std::wstring_view err)
	{
		fmt::wmemory_buffer buf;
		fmt::format_to(buf, fmt(L"Failed to create log file. Logs won't be available during this session.\n\n{}"), err);
		Error::CreateMessageBoxThread(buf, ERROR_TITLE, MB_ICONWARNING);
	}

	template<typename T>
	void write(const T &thing)
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
};

using lazy_file_sink_mt = lazy_file_sink<std::mutex>;
using lazy_file_sink_st = lazy_file_sink<spdlog::details::null_mutex>;
