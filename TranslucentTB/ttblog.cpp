#include "ttblog.hpp"
#include <ctime>
#include <cwchar>
#include <fileapi.h>
#include <fstream>
#include <PathCch.h>
#include <processthreadsapi.h>
#include <sstream>
#include <thread>
#include <wil/safecast.h>
#include <WinBase.h>
#include <winerror.h>
#include <winnt.h>
#include <WinUser.h>

#include "smart/autofree.hpp"
#include "constants.hpp"
#include "win32.hpp"
#include "windows/window.hpp"
#include "util/numbers.hpp"
#include "util/time.hpp"
#include "uwp.hpp"

std::optional<wil::unique_hfile> Log::m_FileHandle;
std::filesystem::path Log::m_File;

#if 0
std::tuple<std::wstring, HRESULT, std::wstring> Log::GetPath()
{
	std::wstring temp;
	temp.resize(LONG_PATH);
	int size = GetTempPath(LONG_PATH, temp.data());
	if (!size)
	{
		return { { }, HRESULT_FROM_WIN32(GetLastError()), L"Failed to determine temporary folder location!" };
	}
	temp.resize(size);

	AutoFree::DebugLocal<wchar_t[]> log_folder_safe;
	const HRESULT hr = PathAllocCombine(temp.c_str(), NAME, PATHCCH_ALLOW_LONG_PATHS, log_folder_safe.put());
	if (FAILED(hr))
	{
		return { { }, hr, L"Failed to combine temporary folder location and app name!" };
	}

	return { log_folder_safe.get(), S_OK, { } };
}
#endif

std::pair<HRESULT, std::wstring> Log::InitStream()
{
	// put this here so that if we fail before creating the file, we won't try constantly doing init.
	m_FileHandle.emplace();

	std::filesystem::path log;
	try
	{
		log = static_cast<std::wstring_view>(UWP::GetApplicationFolderPath(UWP::FolderType::Temporary));
	}
	catch (const winrt::hresult_error &error)
	{
		return { error.code(), L"Failed to determine temporary folder location!" };
	}

	if (!std::filesystem::is_directory(log))
	{
		try
		{
			std::filesystem::create_directory(log);
		}
		catch (const std::filesystem::filesystem_error &err)
		{
			return { HRESULT_FROM_WIN32(err.code().value()), L"Creating log files directory failed!" };
		}
	}

	std::wstring log_filename;
	if (FILETIME creationTime, _, __, ___; GetProcessTimes(GetCurrentProcess(), &creationTime, &_, &__, &___))
	{
		log /= std::to_wstring(win32::FiletimeToUnixEpoch(creationTime)) + L".log";
	}
	else
	{
		// Fallback to current time
		log /= std::to_wstring(Util::GetTime().count()) + L".log";
	}

	m_FileHandle->reset(CreateFile(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL));
	if (!*m_FileHandle)
	{
		return { HRESULT_FROM_WIN32(GetLastError()), L"Failed to create and open log file!" };
	}

	DWORD bytesWritten;
	if (!WriteFile(m_FileHandle->get(), L"\uFEFF", sizeof(wchar_t), &bytesWritten, NULL))
	{
		LastErrorHandle(Error::Level::Debug, L"Failed to write byte-order marker.");
	}

	m_File = std::move(log);
	return { S_OK, { } };
}

void Log::OutputMessage(std::wstring_view message)
{
	if (!init_done())
	{
		auto [hr, err_message] = InitStream();
		if (FAILED(hr))
		{
			// https://stackoverflow.com/questions/50799719/reference-to-local-binding-declared-in-enclosing-function
			std::thread([hr = hr, err = std::move(err_message)]() mutable
			{
				std::wostringstream buffer;
				buffer << err << L" Logs will not be available during this session.\n\n" << Error::ExceptionFromHRESULT(hr);

				err += L'\n';
				OutputDebugString(err.c_str()); // OutputDebugString is thread-safe, no issues using it here.

				MessageBox(Window::NullWindow, buffer.str().c_str(), NAME L" - Error", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
			}).detach();
		}
	}

	OutputDebugString(message.data());
	OutputDebugString(L"\n");

	if (*m_FileHandle)
	{
		const auto time = Util::GetTime<std::time_t>();

		std::wostringstream buffer;
		buffer << L'(' << _wctime(&time);
		buffer.seekp(-1, std::ios_base::end); // Seek behind the newline created by _wctime
		buffer << L") " << message << L"\r\n";

		const std::wstring error = buffer.str();

		DWORD bytesWritten;
		if (!WriteFile(m_FileHandle->get(), error.c_str(), wil::safe_cast<DWORD>(error.length() * sizeof(wchar_t)), &bytesWritten, NULL))
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