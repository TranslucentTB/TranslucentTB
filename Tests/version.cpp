#include <gtest/gtest.h>

#include "version.hpp"

TEST(Version_FromHighLow, ConvertsToSameVersion)
{
	ASSERT_EQ(Version::FromHighLow(0x00010002, 0x00030004), (Version { 1, 2, 3, 4 }));
}

TEST(Version_FromPackageVersion_WinRT, ConvertsToSameVersion)
{
	winrt::Windows::ApplicationModel::PackageVersion winRtVersion = {
		.Major = 1,
		.Minor = 2,
		.Build = 3,
		.Revision = 4
	};

	ASSERT_EQ(Version::FromPackageVersion(winRtVersion), (Version { 1, 2, 3, 4 }));
}

TEST(Version_FromPackageVersion_Win32, ConvertsToSameVersion)
{
	PACKAGE_VERSION win32version = {
		.Revision = 4,
		.Build = 3,
		.Minor = 2,
		.Major = 1
	};

	ASSERT_EQ(Version::FromPackageVersion(win32version), (Version { 1, 2, 3, 4 }));
}

TEST(Version_Format, FormatsProperly)
{
	ASSERT_EQ(std::format(L"{}", Version { 1, 2, 3, 4 }), L"1.2.3.4");
}

TEST(Version_Format, SupportsFormatStrings)
{
	ASSERT_EQ(std::format(L"{:04}", Version { 1, 2, 3, 4 }), L"0001.0002.0003.0004");
}

TEST(Version_Equality, ReturnsTrueWhenSame)
{
	ASSERT_EQ((Version { 1, 2, 3, 4 }), (Version { 1, 2, 3, 4 }));
}

TEST(Version_Equality, ReturnsFalseWhenDifferent)
{
	ASSERT_NE((Version { 1, 2, 3, 4 }), (Version { 5, 6, 7, 8 }));
}
