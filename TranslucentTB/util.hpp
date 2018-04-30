#pragma once
#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <string>
#include <thread>
#include <vector>

class Util {

public:
	inline static void ToLower(std::wstring &data)
	{
		std::transform(data.begin(), data.end(), data.begin(), std::towlower);
	}

	inline static std::wstring Trim(const std::wstring& str, const wchar_t &character = L' ')
	{
		size_t first = str.find_first_not_of(character);

		if (first == std::wstring::npos)
		{
			return L"";
		}

		size_t last = str.find_last_not_of(character);
		return str.substr(first, (last - first + 1));
	}

	inline static void QuoteSpaces(std::wstring &path)
	{
		if (path.find(L' ') != std::wstring::npos)
		{
			path = L"\"" + path + L"\"";
		}
	}

	// For std::bind magic
	template<typename T>
	inline static void UpdateValue(T &toupdate, T newvalue, unsigned int)
	{
		toupdate = newvalue;
	}

	inline static void InvertBool(bool &value, unsigned int)
	{
		value = !value;
	}

	// Opens a file in notepad.
	static void EditFile(std::wstring file);

	// Opens a link in the default browser.
	// NOTE: doesn't attempts to validate the link, make sure it's correct.
	static void OpenLink(const std::wstring &link);

	// Opens a color picker.
	// NOTE: use .join() to wait for input, because this doesn't blocks by default.
	static std::thread PickColor(uint32_t &color);

	// Checks if the start menu is open using a COM interface.
	static bool IsStartVisible();
};