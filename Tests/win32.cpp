#include <compare>
#include <gtest/gtest.h>

#include "win32.hpp"
#include "win32version.h"

// to allow ASSERT_EQ with RECT
static bool operator==(const RECT &left, const RECT &right) noexcept
{
	// make sure there's no padding
	static_assert(sizeof(RECT) == 4 * sizeof(LONG));

	return std::memcmp(&left, &right, sizeof(RECT)) == 0;
}

namespace {
	static constexpr std::pair<std::wstring_view, std::wstring_view> differentContentCases[] = {
		{ L"foo", L"foobar" },
		{ L"FOOBAR", L"FOO" },
		{ L"foo", L"foobar" },
		{ L"FOOBAR", L"FOO" },
		{ L"foo", L"bar" },
		{ L"FOO", L"BAR" }
	};

	static constexpr std::pair<std::wstring_view, std::wstring_view> sameContentCases[] = {
		{ L"foo", L"FOO" },
		{ L"FOOBAR", L"foobar" },
		{ L"foo", L"foo" },
		{ L"\u00EB", L"\u00CB" },
		{ L"aAa\u00CB\u00EB\u00CBAaA", L"AaA\u00EB\u00CB\u00EBaAa" }
	};
}

TEST(win32_GetExeLocation, GetsCorrectFileName)
{
	const auto [location, hr] = win32::GetExeLocation();

	ASSERT_TRUE(SUCCEEDED(hr));
	ASSERT_EQ(location.filename(), L"Tests.exe");
}

TEST(win32_GetFixedFileVersion, GetsCorrectFileVersion)
{
	static constexpr Version expected = { EXPECTED_FILE_VERSION };

	const auto [location, hr] = win32::GetExeLocation();
	ASSERT_TRUE(SUCCEEDED(hr));

	const auto [version, hr2] = win32::GetFixedFileVersion(location);
	ASSERT_TRUE(SUCCEEDED(hr2));
	ASSERT_EQ(version, expected);
}

TEST(win32_RectFitsInRect, FalseWhenBiggerInnerRect)
{
	static constexpr std::pair<RECT, RECT> biggerBounds[] = {
		{ { 2, 2, 3, 3 }, { 0, 0, 4, 4 } },
		{ { 1, 1, 4, 4 }, { 0, 2, 3, 3 } },
		{ { 1, 1, 4, 4 }, { 2, 2, 5, 3 } },
		{ { 1, 1, 4, 4 }, { 2, 0, 3, 3 } },
		{ { 1, 1, 4, 4 }, { 2, 2, 3, 5 } }
	};

	for (const auto &rects : biggerBounds)
	{
		ASSERT_FALSE(win32::RectFitsInRect(rects.first, rects.second));
	}
}

TEST(win32_RectFitsInRect, TrueWhenSmallerInnerRect)
{
	static constexpr std::pair<RECT, RECT> smallerBounds[] = {
		{ { 0, 0, 4, 4 }, { 2, 2, 3, 3 } },
		{ { 0, 0, 3, 3 }, { 0, 1, 2, 2 } },
		{ { 0, 0, 3, 3 }, { 1, 1, 3, 2 } },
		{ { 0, 0, 3, 3 }, { 1, 0, 2, 2 } },
		{ { 0, 0, 3, 3 }, { 1, 1, 2, 3 } },
		{ { 0, 0, 1, 1 }, { 0, 0, 1, 1 } }
	};

	for (const auto &rects : smallerBounds)
	{
		ASSERT_TRUE(win32::RectFitsInRect(rects.first, rects.second));
	}
}

TEST(win32_OffsetRect, SupportsXOffset)
{
	static constexpr std::tuple<RECT, RECT, int> cases[] = {
		{ { 10, 10, 20, 20 }, { 5, 10, 15, 20 }, -5 },
		{ { 10, 10, 20, 20 }, { 15, 10, 25, 20 }, 5 },
		{ { 10, 10, 20, 20 }, { 10, 10, 20, 20 }, 0 }
	};

	for (auto [initial, expected, offset] : cases)
	{
		win32::OffsetRect(initial, offset, 0);
		ASSERT_EQ(initial, expected);
	}
}

TEST(win32_OffsetRect, SupportsYOffset)
{
	static constexpr std::tuple<RECT, RECT, int> cases[] = {
		{ { 10, 10, 20, 20 }, { 10, 5, 20, 15 }, -5 },
		{ { 10, 10, 20, 20 }, { 10, 15, 20, 25 }, 5 },
		{ { 10, 10, 20, 20 }, { 10, 10, 20, 20 }, 0 }
	};

	for (auto [initial, expected, offset] : cases)
	{
		win32::OffsetRect(initial, 0, offset);
		ASSERT_EQ(initial, expected);
	}
}

TEST(win32_IsSameFilename, ReturnsFalse)
{
	for (const auto &testCase : differentContentCases)
	{
		ASSERT_FALSE(win32::IsSameFilename(testCase.first, testCase.second));
	}
}

TEST(win32_IsSameFilename, ReturnsTrue)
{
	for (const auto &testCase : sameContentCases)
	{
		ASSERT_TRUE(win32::IsSameFilename(testCase.first, testCase.second));
	}
}

TEST(win32_FilenameHash, DifferentHash)
{
	const win32::FilenameHash hasher;

	for (const auto &testCase : differentContentCases)
	{
		ASSERT_NE(hasher(testCase.first), hasher(testCase.second));
	}
}

TEST(win32_FilenameHash, SameHash)
{
	const win32::FilenameHash hasher;

	for (const auto &testCase : sameContentCases)
	{
		ASSERT_EQ(hasher(testCase.first), hasher(testCase.second));
	}
}
