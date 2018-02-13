#pragma once
#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <cwchar>
#include <string>
#include <vector>

#include <comdef.h>
#include <Processthreadsapi.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <Synchapi.h>
#include <winbase.h>
#include <windef.h>

#include "app.hpp"
#include "ttberror.hpp"

namespace Util {

	void ToLower(std::wstring &data)
	{
		std::transform(data.begin(), data.end(), data.begin(), ::towlower);
	}

	std::wstring Trim(const std::wstring& str)
	{
		size_t first = str.find_first_not_of(' ');
		size_t last = str.find_last_not_of(' ');

		if (first == std::wstring::npos)
		{
			return std::wstring(L"");
		}
		return str.substr(first, (last - first + 1));
	}

	void EditFile(std::wstring file)
	{
		// WinAPI reeeeeeeeeeeeeeeeeeeeeeeeee
		LPWSTR system32;
		if (!Error::Handle(SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, NULL, &system32), Error::Level::Error, L"Failed to determine System32 folder location!"))
		{
			return;
		}

		wchar_t notepad[MAX_PATH];
		PathCombine(notepad, system32, L"notepad.exe");

		std::vector<wchar_t> buf(file.begin(), file.end());
		buf.push_back(0); // Null terminator
		PathQuoteSpaces(buf.data());

		std::wstring path;
		path += notepad;
		path += ' ';
		path += buf.data();

		std::vector<wchar_t> buf2(path.begin(), path.end());
		buf2.push_back(0); // Null terminator

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
		STARTUPINFO si = { sizeof(si) };
#pragma clang diagnostic pop

		PROCESS_INFORMATION pi;
		// Not using lpApplicationName here because if someone has set a redirect to another editor it doesn't works. (eg Notepad2)
		if (CreateProcess(NULL, buf2.data(), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			Error::Handle(HRESULT_FROM_WIN32(GetLastError()), Error::Level::Error, L"Failed to start Notepad!");
		}
	}

	void AddValuesToVectorByDelimiter(std::wstring delimiter, std::vector<std::wstring> &vector, std::wstring line)
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
