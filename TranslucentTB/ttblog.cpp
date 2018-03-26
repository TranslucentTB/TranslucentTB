#include "ttblog.hpp"
#include <comdef.h>
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

#include "app.hpp"
#include "AutoFreeBase.hpp"
#include "win32.hpp"
#ifdef STORE
#include "UWP.hpp"
#else
#include "common.h"
#endif

std::unique_ptr<std::wostream> Log::m_LogStream;
std::wstring Log::m_File;

inline std::tuple<HRESULT, std::wstring> Log::InitStream()
{
	HRESULT hr;
#ifndef STORE
	std::vector<wchar_t> temp(LONG_PATH);
	if (!GetTempPath(LONG_PATH, temp.data()))
	{
		return std::make_tuple(HRESULT_FROM_WIN32(GetLastError()), L"Failed to determine temporary folder location!");
	}

	AutoFreeBase<wchar_t, SilentLocalFreeDeleter> log_folder;
	hr = PathAllocCombine(temp.data(), App::NAME.c_str(), PATHCCH_ALLOW_LONG_PATHS, &log_folder);
	if (FAILED(hr))
	{
		return std::make_tuple(hr, L"Failed to combine temporary folder location and app name!");
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
			return std::make_tuple(HRESULT_FROM_WIN32(GetLastError()), L"Creating log files directory failed!");
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

	AutoFreeBase<wchar_t, SilentLocalFreeDeleter> log_file;
	hr = PathAllocCombine(log_folder, log_filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &log_file);
	if (FAILED(hr))
	{
		return std::make_tuple(hr, L"Failed to combine log folder location and log file name!");
	}

	m_File = log_file;

	m_LogStream.reset(new std::wofstream(log_file));
	return std::make_tuple(S_OK, L"");

#ifdef STORE
	}
	catch (const winrt::hresult_error &error)
	{
		return std::make_tuple(error.code(), L"Failed to determine temporary folder location!");
	}
#endif
}

std::wstring Log::file()
{
	return m_File;
}

void Log::OutputMessage(const std::wstring &message)
{
	if (m_LogStream.get() == nullptr)
	{
		auto result = InitStream();
		HRESULT hr = std::get<0>(result);
		if (FAILED(hr))
		{
			m_LogStream.reset(new std::wostringstream());

			std::wstring error_message = std::get<1>(result);
			std::wstring boxbuffer = error_message +
				L" Logs will not be available during this session.\n\nException from HRESULT: "
				+ _com_error(hr).ErrorMessage();

			MessageBox(NULL, boxbuffer.c_str(), (App::NAME + L" - Error").c_str(), MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
		}
	}

	std::time_t current_time = std::time(0);

	std::wstring buffer;
	buffer += '(';
	buffer += _wctime(&current_time);
	buffer = buffer.substr(0, buffer.length() - 1);
	buffer += L") ";
	buffer += message;

	*m_LogStream << buffer << std::endl;

	OutputDebugString((message + L"\n").c_str());
}