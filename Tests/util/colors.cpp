#include <gtest/gtest.h>

#include "util/colors.hpp"

using namespace testing;

TEST(Util_ColorFromString, Parses3DigitColor)
{
	ASSERT_EQ(Util::ColorFromString(L"#FAF"), 0xFFAAFF);
}

TEST(Util_ColorFromString, Parses6DigitColor)
{
	ASSERT_EQ(Util::ColorFromString(L"#C0FFEE"), 0xC0FFEE);
}

TEST(Util_ColorFromString, TrimsInput)
{
	ASSERT_EQ(Util::ColorFromString(L"   #FFFFFF \t \n"), 0xFFFFFF);
}

TEST(Util_ColorFromString, ThrowsWhenColorDoesntStartsWithPrefix)
{
	ASSERT_THROW(Util::ColorFromString(L"FFFFFF"), std::invalid_argument);
}

TEST(Util_ColorFromString, ThrowsWhenColorIsNot3Or6Characters)
{
	ASSERT_THROW(Util::ColorFromString(L"#FFFFFFF"), std::invalid_argument);
}

TEST(Util_StringFromColor, ReturnsCorrectString)
{
	ASSERT_EQ(Util::StringFromColor(0xC0FFEE), L"#C0FFEE");
}

TEST(Util_StringFromColor, IgnoresFirstByte)
{
	ASSERT_EQ(Util::StringFromColor(0xDEADBEEF), L"#ADBEEF");
}

TEST(Util_StringFromColor, PadsLeftRight)
{
	ASSERT_EQ(Util::StringFromColor(0x00FF00), L"#00FF00");
}

TEST(Util_StringFromColor, BlackColor)
{
	ASSERT_EQ(Util::StringFromColor(0x000000), L"#000000");
}

TEST(Util_SwapColorEndian, SwapsEndianness)
{
	ASSERT_EQ(Util::SwapColorEndian(0xC0FFEE), 0xEEFFC0);
}

TEST(Util_SwapColorEndian, IgnoresFirstByte)
{
	ASSERT_EQ(Util::SwapColorEndian(0xDEADBEEF), 0xEFBEAD);
}
