#pragma once
#include "arch.h"
#include <fileapi.h>
#include <filesystem>
#include <fmt/format.h>
#include <mutex>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <string_view>
#include <thread>
#include <wil/resource.h>
#include <wil/safecast.h>

#include "constants.hpp"
#include "window.hpp"
#include "ttberror.hpp"

template<typename Mutex>
class lazy_file_sink final : public spdlog::sinks::base_sink<Mutex> {
public:
	explicit lazy_file_sink(std::filesystem::path file) : m_Tried(false), m_File(std::move(file)) { }

	const std::filesystem::path &file() const noexcept { return m_File; }
	bool opened() const noexcept { return bool(m_Handle); }
	bool error() const noexcept { return m_Tried && !m_Handle; }

protected:
	void sink_it_(const spdlog::details::log_msg &msg) override
	{
		open();

		if (m_Handle)
		{
			fmt::memory_buffer formatted;
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
	bool m_Tried;
	wil::unique_hfile m_Handle;
	std::filesystem::path m_File;

	void open()
	{
		if (!m_Tried && !m_Handle)
		{
			m_Handle.reset(CreateFile(m_File.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
			if (m_Handle)
			{
				write<std::string_view>(UTF8_BOM);
			}
			else
			{
				HRESULT err = HRESULT_FROM_WIN32(GetLastError());
				LastErrorHandle(spdlog::level::trace, L"Failed to create log file.");
				std::thread([err]()
				{
					const std::wstring msg =
						fmt::format(fmt(APP_NAME L" tried to log a message but the log file could not be created. Logs won't be available during this session.\n\n{}"), Error::MessageFromHRESULT(err));

					MessageBox(Window::NullWindow, msg.c_str(), APP_NAME L" - Error", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
				}).detach();
			}

			m_Tried = true;
		}
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