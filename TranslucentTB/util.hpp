#pragma once
#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <string>

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
		HRESULT error = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, NULL, &system32);
		if (FAILED(error))
		{
			std::wstring message;
			message += L"Failed to determine System32 folder location!\n\nException from HRESULT: ";
			message += _com_error(error).ErrorMessage();

			MessageBox(NULL, message.c_str(), (std::wstring(App::NAME) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK);

			return;
		}

		TCHAR notepad[MAX_PATH];
		PathCombine(notepad, system32, L"notepad.exe");

		std::vector<TCHAR> buf(file.begin(), file.end());
		buf.push_back(0); // Null terminator
		PathQuoteSpaces(buf.data());

		std::wstring path;
		path += notepad;
		path += ' ';
		path += buf.data();

		std::vector<TCHAR> buf2(path.begin(), path.end());
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
			error = GetLastError();
			std::wstring message;
			message += L"Failed to start Notepad!\n\nException from HRESULT: ";
			message += _com_error(error).ErrorMessage();

			MessageBox(NULL, message.c_str(), (std::wstring(App::NAME) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK);
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
