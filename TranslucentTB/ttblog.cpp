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
#include "window.hpp"
#include "util.hpp"
#ifdef STORE
#include "uwp.hpp"
#endif

std::mutex Log::m_LogLock;
std::optional<winrt::file_handle> Log::m_FileHandle;
std::wstring Log::m_File;

#ifndef STORE
std::tuple<std::wstring, HRESULT, std::wstring> Log::GetPath()
{
	std::wstring temp;
	temp.resize(LONG_PATH);
	int size = GetTempPath(LONG_PATH, temp.data());
	if (!size)
	{
		return { L"", HRESULT_FROM_WIN32(GetLastError()), L"Failed to determine temporary folder location!" };
	}
	temp.resize(size);

	AutoFree::DebugLocal<wchar_t[]> log_folder_safe;
	const HRESULT hr = PathAllocCombine(temp.c_str(), NAME, PATHCCH_ALLOW_LONG_PATHS, log_folder_safe.put());
	if (FAILED(hr))
	{
		return { L"", hr, L"Failed to combine temporary folder location and app name!" };
	}

	return { log_folder_safe.get(), S_OK, L"" };
}
#else
std::tuple<std::wstring, HRESULT, std::wstring> Log::GetPath()
{
	try
	{
		return { UWP::GetApplicationFolderPath(UWP::FolderType::Temporary).c_str(), S_OK, L"" };
	}
	catch (const winrt::hresult_error &error)
	{
		return { L"", error.code(), L"Failed to determine temporary folder location!" };
	}
}
#endif

std::pair<HRESULT, std::wstring> Log::InitStream()
{
	m_FileHandle.emplace(); // put this here so that if we fail before creating the file, we won't try constantly doing init.

	auto [log_folder, hr, err] = GetPath();
	if (FAILED(hr))
	{
		return { hr, std::move(err) };
	}

	if (!win32::IsDirectory(log_folder))
	{
		if (!CreateDirectory(log_folder.c_str(), NULL))
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
		log_filename = std::to_wstring(win32::FiletimeToUnixEpoch(creationTime)) + L".log";
	}
	else
	{
		// Fallback to current time
		log_filename = std::to_wstring(Util::GetCurrentTime<>().count()) + L".log";
	}

	AutoFree::DebugLocal<wchar_t[]> log_file;
	hr = PathAllocCombine(log_folder.c_str(), log_filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, log_file.put());
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
}

void Log::OutputMessage(const std::wstring &message)
{
	std::lock_guard guard(m_LogLock);

	if (!init_done())
	{
		const auto [hr, err_message] = InitStream();
		if (FAILED(hr))
		{
			// https://stackoverflow.com/questions/50799719/reference-to-local-binding-declared-in-enclosing-function
			std::thread([hr = hr, err_message = err_message]() mutable
			{
				std::wstring boxbuffer = err_message +
				L" Logs will not be available during this session.\n\n" + Error::ExceptionFromHRESULT(hr);

				err_message += L'\n';
				OutputDebugString(err_message.c_str()); // OutputDebugString is thread-safe, no issues using it here.

				MessageBox(Window::NullWindow, boxbuffer.c_str(), NAME L" - Error", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
			}).detach();
		}
	}

	OutputDebugString((message + L'\n').c_str());

	if (*m_FileHandle)
	{
		const long long time = Util::GetCurrentTime<>().count();

		std::wostringstream buffer;
		buffer << L'(' << _wctime(&time);
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
	std::lock_guard guard(m_LogLock);

	if (!FlushFileBuffers(m_FileHandle->get()))
	{
		LastErrorHandle(Error::Level::Debug, L"Flusing log file buffer failed.");
	}
}