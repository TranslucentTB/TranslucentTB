#pragma once
#include "arch.h"
#include <compare>
#include <cstdint>
#include <format>
#include <string>
#include <windef.h>
#include <appmodel.h>
#include "winrt.hpp"
#include <winrt/Windows.ApplicationModel.h>

struct Version {
	uint16_t Major;
	uint16_t Minor;
	uint16_t Build;
	uint16_t Revision;

	inline std::wstring ToString() const
	{
		return std::format(L"{}.{}.{}.{}", Major, Minor, Build, Revision);
	}

	static constexpr Version FromHighLow(DWORD high, DWORD low)
	{
		return { HIWORD(high), LOWORD(high), HIWORD(low), LOWORD(low) };
	}

	static constexpr Version FromPackageVersion(winrt::Windows::ApplicationModel::PackageVersion version)
	{
		return { version.Major, version.Minor, version.Build, version.Revision };
	}

	static constexpr Version FromPackageVersion(PACKAGE_VERSION version)
	{
		return { version.Major, version.Minor, version.Build, version.Revision };
	}

	auto operator<=>(const Version &) const = default;
};
