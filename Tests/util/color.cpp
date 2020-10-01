#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "util/color.hpp"
#include "util/to_string_view.hpp"

using namespace testing;

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
	fmt::wmemory_buffer buf;
	Util::Color(0xDE, 0xAD, 0xBE, 0xEF).ToString(buf);
	ASSERT_EQ(Util::ToStringView(buf), L"#DEADBEEF");
}

TEST(Util_Color_ToString, PadsLeftRight)
{
	fmt::wmemory_buffer buf;
	Util::Color(0x00, 0x00, 0xFF, 0x00).ToString(buf);
	ASSERT_EQ(Util::ToStringView(buf), L"#0000FF00");
}

TEST(Util_Color_ToString, TransparentBlackColor)
{
	fmt::wmemory_buffer buf;
	Util::Color(0x00, 0x00, 0x00, 0x00).ToString(buf);
	ASSERT_EQ(Util::ToStringView(buf), L"#00000000");
}

TEST(Util_Color_FromString, Parses3DigitColor)
{
	ASSERT_EQ(Util::Color::FromString(L"#FAF"), Util::Color(0xFF, 0xAA, 0xFF));
}

TEST(Util_Color_FromString, Parses4DigitColor)
{
	ASSERT_EQ(Util::Color::FromString(L"#DEAD"), Util::Color(0xDD, 0xEE, 0xAA, 0xDD));
}

TEST(Util_Color_FromString, Parses6DigitColor)
{
	ASSERT_EQ(Util::Color::FromString(L"#C0FFEE"), Util::Color(0xC0, 0xFF, 0xEE));
}

TEST(Util_Color_FromString, Parses8DigitColor)
{
	ASSERT_EQ(Util::Color::FromString(L"#DEADBEEF"), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_FromString, TrimsInput)
{
	ASSERT_EQ(Util::Color::FromString(L"   #FFFFFF \t \n"), Util::Color(0xFF, 0xFF, 0xFF));
}

TEST(Util_Color_FromString, ThrowsWhenColorDoesntStartsWithPrefix)
{
	ASSERT_THROW(Util::Color::FromString(L"FFFFFF"), std::invalid_argument);
}

TEST(Util_Color_FromString, ThrowsWhenColorIsNotValidCharacterCount)
{
	ASSERT_THROW(Util::Color::FromString(L"#FFFFFFF"), std::invalid_argument);
}

TEST(Util_Color_ToWinRT, ConvertsToSameColor)
{
	const winrt::Windows::UI::Color convertedCol = Util::Color { 0xDE, 0xAD, 0xBE, 0xEF };
	const winrt::Windows::UI::Color originalCol = { .A = 0xEF, .R = 0xDE, .G = 0xAD, .B = 0xBE };

	ASSERT_EQ(convertedCol, originalCol);
}

TEST(Util_Color_Equality, ReturnsTrueWhenSame)
{
	ASSERT_THAT(Util::Color(0xDE, 0xAD, 0xBE, 0xEF) == Util::Color(0xDE, 0xAD, 0xBE, 0xEF), IsTrue());
}

TEST(Util_Color_Equality, ReturnsFalseWhenDifferent)
{
	ASSERT_THAT(Util::Color(0xDE, 0xAD, 0xBE, 0xEF) == Util::Color(0xC0, 0xFF, 0xEE, 0x00), IsFalse());
}
