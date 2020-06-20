#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "config/color.hpp"
#include "util/to_string_view.hpp"

using namespace testing;

TEST(Color_Constructor, DefaultConstructorIsTransparentBlack)
{
	ASSERT_EQ(Color(), Color(0x00, 0x00, 0x00, 0x00));
}

TEST(Color_Constructor, ConstructorFromRGBUsesFullAlpha)
{
	ASSERT_EQ(Color(0x00, 0x00, 0x00), Color(0x00, 0x00, 0x00, 0xFF));
}

TEST(Color_Constructor, ConstructorFromRGBAGivesCorrectValue)
{
	const Color col = { 0xDE, 0xAD, 0xBE, 0xEF };
	ASSERT_EQ(col.R, 0xDE);
	ASSERT_EQ(col.G, 0xAD);
	ASSERT_EQ(col.B, 0xBE);
	ASSERT_EQ(col.A, 0xEF);
}

TEST(Color_Constructor, ConstructorFromWinRTGivesCorrectValue)
{
	const winrt::Windows::UI::Color winrtCol = { .A = 0xEF, .R = 0xDE, .G = 0xAD, .B = 0xBE };
	ASSERT_EQ(Color(winrtCol), Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Color_Constructor, ConstructorFromUInt32GivesCorrectValue)
{
	ASSERT_EQ(Color(0xDEADBEEF), Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Color_ToRGBA, ReturnsCorrectValue)
{
	ASSERT_EQ(Color(0xDE, 0xAD, 0xBE, 0xEF).ToRGBA(), 0xDEADBEEF);
}

TEST(Color_ToABGR, ReturnsCorrectValue)
{
	ASSERT_EQ(Color(0xDE, 0xAD, 0xBE, 0xEF).ToABGR(), 0xEFBEADDE);
}

TEST(Color_ToString, ReturnsCorrectString)
{
	fmt::wmemory_buffer buf;
	Color(0xDE, 0xAD, 0xBE, 0xEF).ToString(buf);
	ASSERT_EQ(Util::ToStringView(buf), L"#DEADBEEF");
}

TEST(Color_ToString, PadsLeftRight)
{
	fmt::wmemory_buffer buf;
	Color(0x00, 0x00, 0xFF, 0x00).ToString(buf);
	ASSERT_EQ(Util::ToStringView(buf), L"#0000FF00");
}

TEST(Color_ToString, TransparentBlackColor)
{
	fmt::wmemory_buffer buf;
	Color(0x00, 0x00, 0x00, 0x00).ToString(buf);
	ASSERT_EQ(Util::ToStringView(buf), L"#00000000");
}

TEST(Color_FromString, Parses3DigitColor)
{
	ASSERT_EQ(Color::FromString(L"#FAF"), Color(0xFF, 0xAA, 0xFF));
}

TEST(Color_FromString, Parses4DigitColor)
{
	ASSERT_EQ(Color::FromString(L"#DEAD"), Color(0xDD, 0xEE, 0xAA, 0xDD));
}

TEST(Color_FromString, Parses6DigitColor)
{
	ASSERT_EQ(Color::FromString(L"#C0FFEE"), Color(0xC0, 0xFF, 0xEE));
}

TEST(Color_FromString, Parses8DigitColor)
{
	ASSERT_EQ(Color::FromString(L"#DEADBEEF"), Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Color_FromString, TrimsInput)
{
	ASSERT_EQ(Color::FromString(L"   #FFFFFF \t \n"), Color(0xFF, 0xFF, 0xFF));
}

TEST(Color_FromString, ThrowsWhenColorDoesntStartsWithPrefix)
{
	ASSERT_THROW(Color::FromString(L"FFFFFF"), std::invalid_argument);
}

TEST(Color_FromString, ThrowsWhenColorIsNotValidCharacterCount)
{
	ASSERT_THROW(Color::FromString(L"#FFFFFFF"), std::invalid_argument);
}

TEST(Color_ToWinRT, ConvertsToSameColor)
{
	const winrt::Windows::UI::Color convertedCol = Color { 0xDE, 0xAD, 0xBE, 0xEF };
	const winrt::Windows::UI::Color originalCol = { .A = 0xEF, .R = 0xDE, .G = 0xAD, .B = 0xBE };

	ASSERT_EQ(convertedCol, originalCol);
}

TEST(Color_Equality, ReturnsTrueWhenSame)
{
	ASSERT_THAT(Color(0xDE, 0xAD, 0xBE, 0xEF) == Color(0xDE, 0xAD, 0xBE, 0xEF), IsTrue());
}

TEST(Color_Equality, ReturnsFalseWhenDifferent)
{
	ASSERT_THAT(Color(0xDE, 0xAD, 0xBE, 0xEF) == Color(0xC0, 0xFF, 0xEE, 0x00), IsFalse());
}

TEST(Color_Inequality, ReturnsTrueWhenDifferent)
{
	ASSERT_THAT(Color(0xDE, 0xAD, 0xBE, 0xEF) != Color(0xC0, 0xFF, 0xEE, 0x00), IsTrue());
}

TEST(Color_Inequality, ReturnsFalseWhenSame)
{
	ASSERT_THAT(Color(0xDE, 0xAD, 0xBE, 0xEF) != Color(0xDE, 0xAD, 0xBE, 0xEF), IsFalse());
}
