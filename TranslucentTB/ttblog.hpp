#pragma once
#ifndef TTBLOG_HPP
#define TTBLOG_HPP

#include <ctime>
#include <cwchar>
#include <fileapi.h>
#include <fstream>
#include <memory>
#include <PathCch.h>
#include <sstream>
#include <string>
#include <tuple>
#ifndef STORE
#include <vector>
#endif

#include "AutoFreeBase.hpp"
#include "app.hpp"
#ifdef STORE
#include "UWP.hpp"
#endif


class Log {

private:
	static std::unique_ptr<std::wostream> m_LogStream;
	static std::tuple<HRESULT, std::wstring> InitStream();
	struct LocalFreeUnsafeDeleter {

		inline void operator() (void *data)
		{
			LocalFree(data);
		}

	};

public:
	// If you set this outside of the class I'll burn you alive.
	static std::wstring File;
	static void OutputMessage(const std::wstring &message);
};



std::unique_ptr<std::wostream> Log::m_LogStream;
std::wstring Log::File;

inline std::tuple<HRESULT, std::wstring> Log::InitStream()
{
	HRESULT hr;
#ifndef STORE
	std::vector<wchar_t> temp(33000);
	if (!GetTempPath(33000, temp.data()))
	{
		return std::make_tuple(HRESULT_FROM_WIN32(GetLastError()), L"Failed to determine temporary folder location!");
	}

	AutoFreeBase<wchar_t, LocalFreeUnsafeDeleter> log_folder;
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

	DWORD attributes = GetFileAttributes(log_folder);
	if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		if (!CreateDirectory(log_folder, NULL))
		{
			return std::make_tuple(HRESULT_FROM_WIN32(GetLastError()), L"Creating log files directory failed!");
		}
	}

	std::time_t unix_epoch = std::time(0);
	std::wstring log_filename = std::to_wstring(unix_epoch) + L".log";

	AutoFreeBase<wchar_t, LocalFreeUnsafeDeleter> log_file;
	hr = PathAllocCombine(log_folder, log_filename.c_str(), PATHCCH_ALLOW_LONG_PATHS, &log_file);
	if (FAILED(hr))
	{
		return std::make_tuple(hr, L"Failed to combine log folder location and log file name!");
	}

	File = log_file;

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
			std::wstring boxbuffer = error_message + L" Logs will not be available during this session.\n\nException from HRESULT: "
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

#endif // !TTBLOG_HPP
