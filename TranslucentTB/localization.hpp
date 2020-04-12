#pragma once
#include "locales.h"
#include <string>
class Localization
{
public:
	Localization(const unsigned int resid);
	~Localization() = default;

private:
	std::wstring entries[::TOTAL_LOCALE_ENTRIES];

public:
	inline const std::wstring& operator[](unsigned int entryid)const { return entries[entryid]; }
};

Localization GetDefaultLocalization();