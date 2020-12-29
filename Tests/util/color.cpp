#include <gtest/gtest.h>

#include "util/color.hpp"
#include "util/to_string_view.hpp"

TEST(Util_Color_Constructor, DefaultConstructorIsTransparentBlack)
{
	ASSERT_EQ(Util::Color(), Util::Color(0x00, 0x00, 0x00, 0x00));
}

TEST(Util_Color_Constructor, ConstructorFromRGBUsesFullAlpha)
{
	ASSERT_EQ(Util::Color(0x00, 0x00, 0x00), Util::Color(0x00, 0x00, 0x00, 0xFF));
}

TEST(Util_Color_Constructor, ConstructorFromRGBAGivesCorrectValue)
{
	const Util::Color col = { 0xDE, 0xAD, 0xBE, 0xEF };
	ASSERT_EQ(col.R, 0xDE);
	ASSERT_EQ(col.G, 0xAD);
	ASSERT_EQ(col.B, 0xBE);
	ASSERT_EQ(col.A, 0xEF);
}

TEST(Util_Color_Constructor, ConstructorFromWinRTGivesCorrectValue)
{
	const winrt::Windows::UI::Color winrtCol = { .A = 0xEF, .R = 0xDE, .G = 0xAD, .B = 0xBE };
	ASSERT_EQ(Util::Color(winrtCol), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_Constructor, ConstructorFromUInt32GivesCorrectValue)
{
	ASSERT_EQ(Util::Color(0xDEADBEEF), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_ToRGBA, ReturnsCorrectValue)
{
	ASSERT_EQ(Util::Color(0xDE, 0xAD, 0xBE, 0xEF).ToRGBA(), 0xDEADBEEF);
}

TEST(Util_Color_ToABGR, ReturnsCorrectValue)
{
	ASSERT_EQ(Util::Color(0xDE, 0xAD, 0xBE, 0xEF).ToABGR(), 0xEFBEADDE);
}

TEST(Util_Color_ToString, ReturnsCorrectString)
{
	static constexpr std::pair<Util::Color, std::wstring_view> cases[] = {
		{ { 0xDE, 0xAD, 0xBE, 0xEF }, L"#DEADBEEF" },
		{ { 0x00, 0x00, 0xFF, 0x00 }, L"#0000FF00" },
		{ { 0x00, 0x00, 0x00, 0x00 }, L"#00000000" },
		{ { 0xFF, 0xFF, 0xFF, 0xFF }, L"#FFFFFFFF" },
	};

	for (const auto &testCase : cases)
	{
		fmt::wmemory_buffer buf;
		testCase.first.ToString(buf);
		ASSERT_EQ(Util::ToStringView(buf), testCase.second);
	}
}

TEST(Util_Color_FromString, ParsesColor)
{
	static constexpr std::pair<std::wstring_view, Util::Color> cases[] = {
		{ L"#FAF", { 0xFF, 0xAA, 0xFF } },
		{ L"#DEAD", { 0xDD, 0xEE, 0xAA, 0xDD } },
		{ L"#C0FFEE", { 0xC0, 0xFF, 0xEE } },
		{ L"#DEADBEEF", { 0xDE, 0xAD, 0xBE, 0xEF } },
		{ L"   #FFFFFF \t \n", { 0xFF, 0xFF, 0xFF }}
	};

	for (const auto &testCase : cases)
	{
		ASSERT_EQ(Util::Color::FromString(testCase.first), testCase.second);
	}
}

TEST(Util_Color_FromString, ThrowsWhenInvalidColor)
{
	static constexpr std::wstring_view cases[] = {
		L"FFFFFF",
		L"#FFFFFFF",
		L"#",
		L"",
		L"  \n \t \r #   \n"
	};

	for (const auto &testCase : cases)
	{
		ASSERT_THROW(Util::Color::FromString(testCase), std::invalid_argument);
	}
}

TEST(Util_Color_ToWinRT, ConvertsToSameColor)
{
	const winrt::Windows::UI::Color convertedCol = Util::Color { 0xDE, 0xAD, 0xBE, 0xEF };
	const winrt::Windows::UI::Color originalCol = { .A = 0xEF, .R = 0xDE, .G = 0xAD, .B = 0xBE };

	ASSERT_EQ(convertedCol, originalCol);
}

TEST(Util_Color_Equality, ReturnsTrueWhenSame)
{
	ASSERT_EQ(Util::Color(0xDE, 0xAD, 0xBE, 0xEF), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_Equality, ReturnsFalseWhenDifferent)
{
	ASSERT_NE(Util::Color(0xDE, 0xAD, 0xBE, 0xEF), Util::Color(0xC0, 0xFF, 0xEE, 0x00));
}
