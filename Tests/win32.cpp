#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "win32.hpp"

using namespace testing;

// to allow ASSERT_EQ with RECT
static bool operator==(const RECT &left, const RECT &right) noexcept
{
	return std::memcmp(&left, &right, sizeof(RECT)) == 0;
}

TEST(win32_RectFitsInRect, FalseWhenBiggerInnerRect)
{
	const RECT a = { 2, 2, 3, 3 };
	const RECT b = { 0, 0, 4, 4 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsFalse());
}

TEST(win32_RectFitsInRect, FalseWhenBiggerLeftBound)
{
	const RECT a = { 1, 1, 4, 4 };
	const RECT b = { 0, 2, 3, 3 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsFalse());
}

TEST(win32_RectFitsInRect, FalseWhenBiggerRightBound)
{
	const RECT a = { 1, 1, 4, 4 };
	const RECT b = { 2, 2, 5, 3 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsFalse());
}

TEST(win32_RectFitsInRect, FalseWhenBiggerTopBound)
{
	const RECT a = { 1, 1, 4, 4 };
	const RECT b = { 2, 0, 3, 3 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsFalse());
}

TEST(win32_RectFitsInRect, FalseWhenBiggerBottomBound)
{
	const RECT a = { 1, 1, 4, 4 };
	const RECT b = { 2, 2, 3, 5 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsFalse());
}

TEST(win32_RectFitsInRect, TrueWhenSmallerInnerRect)
{
	const RECT a = { 0, 0, 4, 4 };
	const RECT b = { 2, 2, 3, 3 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsTrue());
}

TEST(win32_RectFitsInRect, TrueWhenSameLeftBound)
{
	const RECT a = { 0, 0, 3, 3 };
	const RECT b = { 0, 1, 2, 2 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsTrue());
}

TEST(win32_RectFitsInRect, TrueWhenSameRightBound)
{
	const RECT a = { 0, 0, 3, 3 };
	const RECT b = { 1, 1, 3, 2 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsTrue());
}

TEST(win32_RectFitsInRect, TrueWhenSameTopBound)
{
	const RECT a = { 0, 0, 3, 3 };
	const RECT b = { 1, 0, 2, 2 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsTrue());
}

TEST(win32_RectFitsInRect, TrueWhenSameBottomBound)
{
	const RECT a = { 0, 0, 3, 3 };
	const RECT b = { 1, 1, 2, 3 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsTrue());
}

TEST(win32_RectFitsInRect, TrueWhenSameBounds)
{
	const RECT a = { 0, 0, 1, 1 };
	const RECT b = { 0, 0, 1, 1 };

	ASSERT_THAT(win32::RectFitsInRect(a, b), IsTrue());
}

TEST(win32_OffsetRect, SupportsNegativeXOffset)
{
	RECT r = {
		.left = 10,
		.top = 10,
		.right = 20,
		.bottom = 20
	};
	const RECT expected = {
		.left = 5,
		.top = 10,
		.right = 15,
		.bottom = 20
	};

	win32::OffsetRect(r, -5, 0);
	ASSERT_EQ(r, expected);
}

TEST(win32_OffsetRect, SupportsNegativeYOffset)
{
	RECT r = {
		.left = 10,
		.top = 10,
		.right = 20,
		.bottom = 20
	};
	const RECT expected = {
		.left = 10,
		.top = 5,
		.right = 20,
		.bottom = 15
	};

	win32::OffsetRect(r, 0, -5);
	ASSERT_EQ(r, expected);
}

TEST(win32_OffsetRect, SupportsPositiveXOffset)
{
	RECT r = {
		.left = 10,
		.top = 10,
		.right = 20,
		.bottom = 20
	};
	const RECT expected = {
		.left = 15,
		.top = 10,
		.right = 25,
		.bottom = 20
	};

	win32::OffsetRect(r, 5, 0);
	ASSERT_EQ(r, expected);
}

TEST(win32_OffsetRect, SupportsPositiveYOffset)
{
	RECT r = {
		.left = 10,
		.top = 10,
		.right = 20,
		.bottom = 20
	};
	const RECT expected = {
		.left = 10,
		.top = 15,
		.right = 20,
		.bottom = 25
	};

	win32::OffsetRect(r, 0, 5);
	ASSERT_EQ(r, expected);
}

TEST(win32_OffsetRect, SupportsZeroOffset)
{
	RECT r = {
		.left = 10,
		.top = 10,
		.right = 20,
		.bottom = 20
	};
	const RECT expected = r;

	win32::OffsetRect(r, 0, 0);
	ASSERT_EQ(r, expected);
}

TEST(win32_IsSameFilename, FalseWhenLengthDifferent)
{
	ASSERT_THAT(win32::IsSameFilename(L"foo", L"foobar"), IsFalse());
	ASSERT_THAT(win32::IsSameFilename(L"FOOBAR", L"FOO"), IsFalse());
}

TEST(win32_IsSameFilename, FalseWhenContentDifferent)
{
	ASSERT_THAT(win32::IsSameFilename(L"foo", L"bar"), IsFalse());
	ASSERT_THAT(win32::IsSameFilename(L"FOO", L"BAR"), IsFalse());
}

TEST(win32_IsSameFilename, TrueWhenCaseDifferent)
{
	ASSERT_THAT(win32::IsSameFilename(L"foo", L"FOO"), IsTrue());
	ASSERT_THAT(win32::IsSameFilename(L"FOOBAR", L"foobar"), IsTrue());
}

TEST(win32_IsSameFilename, TrueWhenContentSame)
{
	ASSERT_THAT(win32::IsSameFilename(L"foo", L"foo"), IsTrue());
}

TEST(win32_IsSameFilename, HandlesNonAsciiCharacters)
{
	ASSERT_THAT(win32::IsSameFilename(L"\u00EB", L"\u00CB"), IsTrue());
}

TEST(win32_IsSameFilename, HandlesMixedCharacters)
{
	ASSERT_THAT(win32::IsSameFilename(L"aAa\u00CB\u00EB\u00CBAaA", L"AaA\u00EB\u00CB\u00EBaAa"), IsTrue());
}

TEST(win32_FilenameHash, DifferentWhenLengthDifferent)
{
	const win32::FilenameHash hasher;

	ASSERT_NE(hasher(L"foo"), hasher(L"foobar"));
	ASSERT_NE(hasher(L"FOOBAR"), hasher(L"FOO"));
}

TEST(win32_FilenameHash, DifferentWhenContentDifferent)
{
	const win32::FilenameHash hasher;

	ASSERT_NE(hasher(L"foo"), hasher(L"bar"));
	ASSERT_NE(hasher(L"FOO"), hasher(L"BAR"));
}

TEST(win32_FilenameHash, SameWhenCaseDifferent)
{
	const win32::FilenameHash hasher;

	ASSERT_EQ(hasher(L"foo"), hasher(L"FOO"));
	ASSERT_EQ(hasher(L"FOOBAR"), hasher(L"foobar"));
}

TEST(win32_FilenameHash, SameWhenContentSame)
{
	const win32::FilenameHash hasher;

	ASSERT_EQ(hasher(L"foo"), hasher(L"foo"));
}

TEST(win32_FilenameHash, HandlesNonAsciiCharacters)
{
	const win32::FilenameHash hasher;

	ASSERT_EQ(hasher(L"\u00EB"), hasher(L"\u00CB"));
}

TEST(win32_FilenameHash, HandlesMixedCharacters)
{
	const win32::FilenameHash hasher;

	ASSERT_EQ(hasher(L"aAa\u00CB\u00EB\u00CBAaA"), hasher(L"AaA\u00EB\u00CB\u00EBaAa"));
}
