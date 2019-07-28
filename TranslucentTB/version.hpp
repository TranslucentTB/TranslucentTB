#pragma once
#include "arch.h"
#include <cstdint>
#include <fmt/format.h>
#include <string>
#include <windef.h>
#include <winrt/Windows.ApplicationModel.h>

struct Version
{
	uint16_t Major;
	uint16_t Minor;
	uint16_t Build;
	uint16_t Revision;

	inline std::wstring ToString() const
	{
		return fmt::format(fmt(L"{}.{}.{}.{}"), Major, Minor, Build, Revision);
	}

	inline static Version FromHighLow(DWORD high, DWORD low)
	{
		return { HIWORD(high), LOWORD(high), HIWORD(low), LOWORD(low) };
	}

	inline static Version FromPackageVersion(winrt::Windows::ApplicationModel::PackageVersion version)
	{
		return { version.Major, version.Minor, version.Build, version.Revision };
	}
};