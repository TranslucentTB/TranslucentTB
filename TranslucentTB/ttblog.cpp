#include "ttblog.hpp"
#include <ctime>
#include <cwchar>
#include <fileapi.h>
#include <fstream>
#include <PathCch.h>
#include <processthreadsapi.h>
#include <sstream>
#include <thread>
#ifndef STORE
#include <vector>
#endif
#include <WinBase.h>
#include <winerror.h>
#include <winnt.h>
#include <WinUser.h>

#include "autofree.hpp"
#include "common.hpp"
#include "win32.hpp"
#ifdef STORE
#include "uwp.hpp"
#endif

std::mutex Log::m_LogLock;
std::optional<winrt::file_handle> Log::m_FileHandle;
std::wstring Log::m_File;

std::pair<HRESULT, std::wstring> Log::InitStream()
{
	HRESULT hr;
	m_FileHandle.emplace(); // put this here so that if we fail before creating the file, we won't try constantly doing init.
#ifndef STORE
	std::wstring temp;
	temp.resize(LONG_PATH);
	int size = GetTempPath(LONG_PATH, temp.data());
	if (!size)
	{
		return { HRESULT_FROM_WIN32(GetLastError()), L"Failed to determine temporary folder location!" };
	}
	temp.resize(size);

	AutoFree::DebugLocal<wchar_t> log_folder_safe;
	hr = PathAllocCombine(temp.c_str(), NAME, PATHCCH_ALLOW_LONG_PATHS, log_folder_safe.put());
	if (FAILED(hr))
	{
		return { hr, L"Failed to combine temporary folder location and app name!" };
	}
	const wchar_t *log_folder = log_folder_safe.get();
#else
	try
	{
		winrt::hstring tempFolder_str = UWP::GetApplicationFolderPath(UWP::FolderType::Temporary);
		const wchar_t *log_folder = tempFolder_str.c_str();
#endif

	if (!win32::IsDirectory(log_folder))
	{
		if (!CreateDirectory(log_folder, NULL))
		{
			return { HRESULT_FROM_WIN32(GetLastError()), L"Creating log files directory failed!" };
		}
	}

	std::wstring log_filename;
	FILETIME creationTime;
	FILETIME useless1;
	FILETIME useless2;
	FILETIME useless3;
	if (GetProcessTimes(GetCurrentProcess(), &creationTime, &useless1, &useless2, &useless3))
	{
		// Unix timestamps are since 1970, but FILETIME is since 1601 (seriously why MS)
		// FILETIME is also in hundreds of nanoseconds, but Unix timestamps are in seconds.

		// Useful union to convert from a high-word and low-word big integer to a long long.
		LARGE_INTEGER creationTimestamp = {{
			creationTime.dwLowDateTime,
			creationTime.dwHighDateTime
		}};

		// There are 10000000 hundreds of nanoseconds in a second.
		// Convert to seconds.
		creationTimestamp.QuadPart /= 10000000;

		// There are 11644473600 seconds between the two years.
		// Remove the difference.
		creationTimestamp.QuadPart -= 11644473600;

		log_filename = std::to_wstring(creationTimestamp.QuadPart) + L".log";
	}
	else
	{
		// Fallback to current time
		std::time_t unix_epoch = std::time(0);
		log_filename = std::to_wstring(unix_epoch) + L".log";
	}

	AutoFree::DebugLocal<wchar_t> log_file;
	hr = PathAllocCombine(log_folder, log_filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, log_file.put());
	if (FAILED(hr))
	{
		return { hr, L"Failed to combine log folder location and log file name!" };
	}

	m_FileHandle = CreateFile(log_file.get(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (!*m_FileHandle)
	{
		return { HRESULT_FROM_WIN32(GetLastError()), L"Failed to create and open log file!" };
	}

	DWORD bytesWritten;
	if (!WriteFile(m_FileHandle->get(), L"\uFEFF", sizeof(wchar_t), &bytesWritten, NULL))
	{
		LastErrorHandle(Error::Level::Debug, L"Failed to write byte-order marker.");
	}

	m_File = log_file.get();
	return { S_OK, L"" };

#ifdef STORE
	}
	catch (const winrt::hresult_error &error)
	{
		return { error.code(), L"Failed to determine temporary folder location!" };
	}
#endif
}

void Log::OutputMessage(const std::wstring &message)
{
	std::lock_guard guard(m_LogLock);

	if (!init_done())
	{
		auto [hr, err_message] = InitStream();
		if (FAILED(hr))
		{
			// https://stackoverflow.com/questions/50799719/reference-to-local-binding-declared-in-enclosing-function
			std::thread([hr = hr, err_message = err_message]() mutable
			{
				std::wstring boxbuffer = err_message +
				L" Logs will not be available during this session.\n\n" + Error::ExceptionFromHRESULT(hr);

				err_message += L'\n';
				OutputDebugString(err_message.c_str()); // OutputDebugString is thread-safe, no issues using it here.

				MessageBox(NULL, boxbuffer.c_str(), NAME L" - Error", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
			}).detach();
		}
	}

	OutputDebugString((message + L'\n').c_str());

	if (*m_FileHandle)
	{
		std::time_t current_time = std::time(0);

		std::wostringstream buffer;
		buffer << L'(' << _wctime(&current_time);
		buffer.seekp(-1, std::ios_base::end); // Seek behind the newline created by _wctime
		buffer << L") " << message << L"\r\n";

		const std::wstring error = buffer.str();

		DWORD bytesWritten;
		if (!WriteFile(m_FileHandle->get(), error.c_str(), error.length() * sizeof(wchar_t), &bytesWritten, NULL))
		{
			LastErrorHandle(Error::Level::Debug, L"Writing to log file failed.");
		}
	}
}

void Log::Flush()
{
	if (!FlushFileBuffers(m_FileHandle->get()))
	{
		LastErrorHandle(Error::Level::Debug, L"Flusing log file buffer failed.");
	}
}