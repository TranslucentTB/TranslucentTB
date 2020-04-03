#include <gtest/gtest.h>

#include "util/colors.hpp"
#include "util/to_string_view.hpp"

using namespace testing;

TEST(Util_ColorFromString, Parses3DigitColor)
{
	ASSERT_EQ(Util::ColorFromString(L"#FAF"), static_cast<uint32_t>(0xFFAAFF));
}

TEST(Util_ColorFromString, Parses6DigitColor)
{
	ASSERT_EQ(Util::ColorFromString(L"#C0FFEE"), static_cast<uint32_t>(0xC0FFEE));
}

TEST(Util_ColorFromString, TrimsInput)
{
	ASSERT_EQ(Util::ColorFromString(L"   #FFFFFF \t \n"), static_cast<uint32_t>(0xFFFFFF));
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
	fmt::wmemory_buffer buf;
	Util::StringFromColor(buf, 0xC0FFEE);
	ASSERT_EQ(Util::ToStringView(buf), L"#C0FFEE");
}

TEST(Util_StringFromColor, IgnoresFirstByte)
{
	fmt::wmemory_buffer buf;
	Util::StringFromColor(buf, 0xDEADBEEF);
	ASSERT_EQ(Util::ToStringView(buf), L"#ADBEEF");
}

TEST(Util_StringFromColor, PadsLeftRight)
{
	fmt::wmemory_buffer buf;
	Util::StringFromColor(buf, 0x00FF00);
	ASSERT_EQ(Util::ToStringView(buf), L"#00FF00");
}

TEST(Util_StringFromColor, BlackColor)
{
	fmt::wmemory_buffer buf;
	Util::StringFromColor(buf, 0x000000);
	ASSERT_EQ(Util::ToStringView(buf), L"#000000");
}

TEST(Util_SwapColorEndian, SwapsEndianness)
{
	ASSERT_EQ(Util::SwapColorEndian(0xC0FFEE), static_cast<uint32_t>(0xEEFFC0));
}

TEST(Util_SwapColorEndian, IgnoresFirstByte)
{
	ASSERT_EQ(Util::SwapColorEndian(0xDEADBEEF), static_cast<uint32_t>(0xEFBEAD));
}
