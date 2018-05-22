#include "ttblog.hpp"
#include <ctime>
#include <cwchar>
#include <fileapi.h>
#include <fstream>
#include <PathCch.h>
#include <processthreadsapi.h>
#include <sstream>
#ifndef STORE
#include <vector>
#endif
#include <WinBase.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>
#include <WinUser.h>

#include "autofree.hpp"
#include "common.hpp"
#include "win32.hpp"
#ifdef STORE
#include "UWP.hpp"
#endif

std::unique_ptr<File> Log::m_FileHandle;
std::wstring Log::m_File;

inline std::pair<HRESULT, std::wstring> Log::InitStream()
{
	HRESULT hr;
#ifndef STORE
	std::vector<wchar_t> temp(LONG_PATH);
	if (!GetTempPath(LONG_PATH, temp.data()))
	{
		return std::make_pair(HRESULT_FROM_WIN32(GetLastError()), L"Failed to determine temporary folder location!");
	}

	AutoFree::SilentLocal<wchar_t> log_folder;
	hr = PathAllocCombine(temp.data(), NAME, PATHCCH_ALLOW_LONG_PATHS, &log_folder);
	if (FAILED(hr))
	{
		return std::make_pair(hr, L"Failed to combine temporary folder location and app name!");
	}
#else
	try
	{
		std::wstring tempFolder_str = UWP::GetApplicationFolderPath(UWP::FolderType::Temporary);
		const wchar_t *log_folder = tempFolder_str.c_str();
#endif

	if (!win32::IsDirectory(static_cast<const wchar_t *>(log_folder)))
	{
		if (!CreateDirectory(log_folder, NULL))
		{
			return std::make_pair(HRESULT_FROM_WIN32(GetLastError()), L"Creating log files directory failed!");
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
		LARGE_INTEGER creationTimestamp;
		creationTimestamp.HighPart = creationTime.dwHighDateTime;
		creationTimestamp.LowPart = creationTime.dwLowDateTime;

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

	AutoFree::SilentLocal<wchar_t> log_file;
	hr = PathAllocCombine(log_folder, log_filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &log_file);
	if (FAILED(hr))
	{
		return std::make_pair(hr, L"Failed to combine log folder location and log file name!");
	}

	m_File = log_file;

	HANDLE file = CreateFile(log_file, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		return std::make_pair(HRESULT_FROM_WIN32(GetLastError()), L"Failed to create and open log file!");
	}

	DWORD bytesWritten;
	if (!WriteFile(file, L"\uFEFF", sizeof(wchar_t), &bytesWritten, NULL))
	{
		LastErrorHandle(Error::Level::Debug, L"Failed to write byte-order marker.");
	}

	m_FileHandle.reset(new File(file));
	return std::make_pair(S_OK, L"");

#ifdef STORE
	}
	catch (const winrt::hresult_error &error)
	{
		return std::make_pair(error.code(), L"Failed to determine temporary folder location!");
	}
#endif
}

const std::wstring &Log::file()
{
	return m_File;
}

void Log::OutputMessage(const std::wstring &message)
{
	if (m_FileHandle.get() == nullptr)
	{
		auto result = InitStream();
		if (FAILED(result.first))
		{
			m_FileHandle.reset(new File);

			std::wstring error_message = result.second;
			std::wstring boxbuffer = error_message +
				L" Logs will not be available during this session.\n\n" + Error::ExceptionFromHRESULT(result.first);

			MessageBox(NULL, boxbuffer.c_str(), (std::wstring(NAME) + L" - Error").c_str(), MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
		}
	}

	std::time_t current_time = std::time(0);

	std::wstring buffer;
	buffer += L'(';
	buffer += _wctime(&current_time);
	buffer.erase(buffer.length() - 1); // Remove the newline created by _wctime
	buffer += L") ";
	buffer += message;
	buffer += L'\n';

	OutputDebugString((message + L'\n').c_str());

	if (m_FileHandle)
	{
		DWORD bytesWritten;
		if (!WriteFile(*m_FileHandle, buffer.c_str(), buffer.length() * sizeof(wchar_t), &bytesWritten, NULL))
		{
			LastErrorHandle(Error::Level::Debug, L"Writing to log file failed.");
		}
	}
}

void Log::Flush()
{
	if (!FlushFileBuffers(*m_FileHandle))
	{
		LastErrorHandle(Error::Level::Debug, L"Flusing log file buffer failed.");
	}
}