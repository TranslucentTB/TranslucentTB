#pragma once
#include "arch.h"
#include <compare>
#include <cstdint>
#include <format>
#include <string>
#include <windef.h>
#include <winbase.h>
#include <appmodel.h>
#include "winrt.hpp"
#include <winrt/Windows.ApplicationModel.h>

struct Version {
	uint16_t Major;
	uint16_t Minor;
	uint16_t Build;
	uint16_t Revision;

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

template<>
struct std::formatter<Version, wchar_t> : std::formatter<uint16_t, wchar_t> {
private:
	template<typename FormatContext>
	static void insert_dot(FormatContext &fc)
	{
		auto o = fc.out();
		*o = L'.';
		++o;
		fc.advance_to(std::move(o));
	}

public:
	template<typename FormatContext>
	auto format(Version v, FormatContext &fc) const
	{
		std::formatter<uint16_t, wchar_t>::format(v.Major, fc);
		insert_dot(fc);
		std::formatter<uint16_t, wchar_t>::format(v.Minor, fc);
		insert_dot(fc);
		std::formatter<uint16_t, wchar_t>::format(v.Build, fc);
		insert_dot(fc);
		return std::formatter<uint16_t, wchar_t>::format(v.Revision, fc);
	}
};
