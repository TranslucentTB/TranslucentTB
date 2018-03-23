#pragma once
#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <string>
#include <vector>

#include <comdef.h>
#include <processthreadsapi.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <synchapi.h>
#include <WinBase.h>
#include <windef.h>

#include "app.hpp"
#include "AutoFree.hpp"
#include "ttberror.hpp"

#include "../CPicker/CPickerDll.h"

namespace Util {

	void ToLower(std::wstring &data)
	{
		std::transform(data.begin(), data.end(), data.begin(), ::towlower);
	}

	std::wstring Trim(const std::wstring& str)
	{
		size_t first = str.find_first_not_of(' ');

		if (first == std::wstring::npos)
		{
			return L"";
		}

		size_t last = str.find_last_not_of(' ');
		return str.substr(first, (last - first + 1));
	}

	void QuoteSpaces(std::wstring &path)
	{
		if (path.find_first_of(' ') != std::wstring::npos)
		{
			path = L"\"" + path + L"\"";
		}
	}

	void EditFile(std::wstring file)
	{
		// WinAPI reeeeeeeeeeeeeeeeeeeeeeeeee
		AutoCoTaskMemFree<wchar_t> system32;
		if (!ErrorHandle(SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, NULL, &system32), Error::Level::Error, L"Failed to determine System32 folder location!"))
		{
			return;
		}

		AutoLocalFree<wchar_t> notepad;
		if (!ErrorHandle(PathAllocCombine(system32, L"notepad.exe", PATHCCH_ALLOW_LONG_PATHS, &notepad), Error::Level::Error, L"Failed to determine Notepad location!"))
		{
			return;
		}

		std::wstring str_notepad(notepad);

		QuoteSpaces(str_notepad);
		QuoteSpaces(file);
		std::wstring path = str_notepad + L" " + file;

		std::vector<wchar_t> buf(path.begin(), path.end());
		buf.push_back(0); // Null terminator

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
		STARTUPINFO si = { sizeof(si) };
#pragma clang diagnostic pop

		PROCESS_INFORMATION pi;
		// Not using lpApplicationName here because if someone has set a redirect to another editor it doesn't works. (eg Notepad2)
		if (CreateProcess(NULL, buf.data(), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
		{
			if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED)
			{
				ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to wait for Notepad close.");
			}
			if (!CloseHandle(pi.hProcess))
			{
				ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to close process handle.");
			}
			if (!CloseHandle(pi.hThread))
			{
				ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Log, L"Failed to close thread handle.");
			}
		}
		else
		{
			ErrorHandle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to start Notepad!");
		}
	}

	uint32_t PickColor(const uint32_t &color)
	{
		const unsigned short a = (color & 0xFF000000) >> 24;
		const unsigned short r = (color & 0x00FF0000) >> 16;
		const unsigned short g = (color & 0x0000FF00) >> 8;
		const unsigned short b = (color & 0x000000FF);

		// Bet 5 bucks a british wrote this library
		CColourPicker picker(NULL, r, g, b, a, true);
		picker.CreateColourPicker();
		SColour newColor = picker.GetCurrentColour();

		return (newColor.a << 24) + (newColor.r << 16) + (newColor.g << 8) + newColor.b;
	}

	void AddValuesToVectorByDelimiter(const std::wstring &delimiter, std::vector<std::wstring> &vector, std::wstring line)
	{
		size_t pos;
		std::wstring value;

		// First lets remove the key
		if ((pos = line.find(delimiter)) != std::wstring::npos)
		{
			line.erase(0, pos + delimiter.length());
		}

		// Now iterate and add the values
		while ((pos = line.find(delimiter)) != std::wstring::npos)
		{
			value = Trim(line.substr(0, pos));
			vector.push_back(value);
			line.erase(0, pos + delimiter.length());
		}
	}
}

#endif // !UTIL_HPP
