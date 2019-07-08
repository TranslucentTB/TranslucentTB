#include <gtest/gtest.h>

#include "util/colors.hpp"

using namespace testing;

TEST(Util_ColorFromString, Parses3DigitColor)
{
	ASSERT_EQ(Util::ColorFromString(L"#faf"), 0xFFAAFF);
}

TEST(Util_ColorFromString, Parses6DigitColor)
{
	ASSERT_EQ(Util::ColorFromString(L"#c0ffee"), 0xc0ffee);
}

TEST(Util_ColorFromString, TrimsInput)
{
	ASSERT_EQ(Util::ColorFromString(L"   #ffffff \t \n"), 0xFFFFFF);
}

TEST(Util_ColorFromString, ThrowsWhenColorDoesntStartsWithPrefix)
{
	ASSERT_THROW(Util::ColorFromString(L"ffffff"), std::invalid_argument);
}

TEST(Util_ColorFromString, ThrowsWhenColorIsNot3Or6Characters)
{
	ASSERT_THROW(Util::ColorFromString(L"#fffffff"), std::invalid_argument);
}

TEST(Util_StringFromColor, ReturnsCorrectString)
{
	ASSERT_EQ(Util::StringFromColor(0xc0ffee), L"#c0ffee");
}

TEST(Util_StringFromColor, IgnoresFirstByte)
{
	ASSERT_EQ(Util::StringFromColor(0xdeadbeef), L"#adbeef");
}

TEST(Util_StringFromColor, PadsLeftRight)
{
	ASSERT_EQ(Util::StringFromColor(0x00ff00), L"#00ff00");
}

TEST(Util_SwapColorEndian, SwapsEndianness)
{
	ASSERT_EQ(Util::SwapColorEndian(0xc0ffee), 0xeeffc0);
}

TEST(Util_SwapColorEndian, IgnoresFirstByte)
{
	ASSERT_EQ(Util::SwapColorEndian(0xdeadbeef), 0xefbead);
}