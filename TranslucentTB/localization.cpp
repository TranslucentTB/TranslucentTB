#include <Windows.h>

#include "localization.hpp"
#include "resource.h"

#pragma comment(lib,"Kernel32.lib")

Localization GetDefaultLocalization()
{
	LANGID lid = GetSystemDefaultLangID();
	switch (lid)
	{
	default:
		return Localization(/*IDR_FILE_LOCALE_EN_US*/0);
	}
}

std::wstring UTF8toWchar(const char* u8string, int len = -1)
{
	auto size = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, u8string, len, nullptr, 0);
	std::wstring res;
	res.resize(size);
	MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, u8string, len, res.data(), size);
	return res;
}

Localization::Localization(const unsigned int resid)
{
	auto intr = MAKEINTRESOURCE(resid);
	auto hres = ::FindResource(NULL, intr, L"LOCALIZATION_FILE");
	DWORD dwSize = ::SizeofResource(NULL, hres);
	LPBYTE lpRsrc = (LPBYTE)::LoadResource(NULL, hres);

	const char* text = reinterpret_cast<char*>(lpRsrc);

	for (size_t i = 0; i < dwSize; i++)
	{
		auto lineStart = text + i;
		while (*lineStart == ' ')*lineStart++;
		if (*lineStart != '#')
		{
			auto entryName = strstr(text + i, "=");

			while (*lineStart == ' ')*lineStart++;
			while (*(entryName - 1) == ' ')*entryName--;

			auto wsEntryName = UTF8toWchar(lineStart, entryName - lineStart);

			auto entryValueStart = strstr(lineStart, "\"") + 1;
			auto entryValueEnd = strstr(entryValueStart, "\"");

			auto wsEntryValue = UTF8toWchar(entryValueStart, entryValueEnd - entryValueStart);

			auto entryID = (uint32_t)::LOCALE_ENTRIES_MAP.at(wsEntryName);

			this->entries[entryID] = wsEntryValue;
		}
		i = strstr(lineStart, "\n") - text;
	}
}