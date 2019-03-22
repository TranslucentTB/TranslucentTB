#include <cstdint>
#include <gtest/gtest.h>

#include "util/numbers.hpp"

TEST(Util_ClampTo_Unsigned_Signed, ClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<int16_t, uint32_t>(4000000)), 32767);
}

TEST(Util_ClampTo_Unsigned_Signed, DoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<int16_t, uint32_t>(300)), 300);
}

TEST(Util_ClampTo_Unsigned_Unsigned, ClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, uint32_t>(4000000)), 65535);
}

TEST(Util_ClampTo_Unsigned_Unsigned, DoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, uint32_t>(300)), 300);
}

TEST(Util_ClampTo_Signed_Unsigned, ClampsToZeroWhenSourceNegative)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, int32_t>(-2000000)), 0);
}

TEST(Util_ClampTo_Signed_Unsigned, ClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, int32_t>(2000000)), 65535);
}

TEST(Util_ClampTo_Signed_Unsigned, DoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, int32_t>(300)), 300);
}

TEST(Util_ClampTo_Signed_Signed, ClampsToMinimumValueWhenSourceTooSmall)
{
	EXPECT_EQ((Util::ClampTo<int16_t, int32_t>(-2000000)), -32768);
}

TEST(Util_ClampTo_Signed_Signed, ClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<int16_t, int32_t>(2000000)), 32767);
}

TEST(Util_ClampTo_Signed_Signed, DoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<int16_t, int32_t>(300)), 300);
}

TEST(Util_IsDecimalDigit, ReturnsFalseWhenNotDigit)
{
	EXPECT_FALSE(Util::impl::IsDecimalDigit(L'a'));
}

TEST(Util_IsDecimalDigit, ReturnsTrueWhenDigit)
{
	EXPECT_TRUE(Util::impl::IsDecimalDigit(L'5'));
}

TEST(Util_IsCapitalHexDigit, ReturnsFalseWhenNotDigit)
{
	EXPECT_FALSE(Util::impl::IsCapitalHexDigit(L'R'));
}

TEST(Util_IsCapitalHexDigit, ReturnsFalseWhenLowerCaseDigit)
{
	EXPECT_FALSE(Util::impl::IsCapitalHexDigit(L'f'));
}

TEST(Util_IsCapitalHexDigit, ReturnsTrueWhenUpperCaseDigit)
{
	EXPECT_TRUE(Util::impl::IsCapitalHexDigit(L'F'));
}

TEST(Util_IsLowerHexDigit, ReturnsFalseWhenNotDigit)
{
	EXPECT_FALSE(Util::impl::IsLowerHexDigit(L'r'));
}

TEST(Util_IsLowerHexDigit, ReturnsFalseWhenUpperCaseDigit)
{
	EXPECT_FALSE(Util::impl::IsLowerHexDigit(L'F'));
}

TEST(Util_IsLowerHexDigit, ReturnsTrueWhenLowerCaseDigit)
{
	EXPECT_TRUE(Util::impl::IsLowerHexDigit(L'f'));
}

TEST(Util_pow, ReturnsOneWhenPowerIsZero)
{
	EXPECT_EQ(Util::impl::pow<uint64_t>(164, 0), 1);
}

TEST(Util_pow, ReturnsCorrectPowerOfTen)
{
	EXPECT_EQ(Util::impl::pow<uint64_t>(10, 6), 1000000);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, ThrowsWhenInputNegative)
{
	EXPECT_THROW((Util::ParseNumber<uint32_t, 10>(L"-10")), std::out_of_range);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, ReturnsCorrectValueWhenInputPositive)
{
	EXPECT_EQ((Util::ParseNumber<uint32_t, 10>(L"1000")), 1000);
}

TEST(Util_ParseNumber_Signed_BaseTen, ReturnsCorrectValueWhenInputNegative)
{
	EXPECT_EQ((Util::ParseNumber<int32_t, 10>(L"-1000")), -1000);
}

TEST(Util_ParseNumber_Signed_BaseTen, ReturnsCorrectValueWhenInputPositive)
{
	EXPECT_EQ((Util::ParseNumber<int32_t, 10>(L"1000")), 1000);
}

TEST(Util_ParseNumber_BaseTen, ThrowsWhenInputNotANumber)
{
	EXPECT_THROW((Util::ParseNumber<int32_t, 10>(L"foobar")), std::invalid_argument);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenLowerCaseDigits)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"ff")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenUpperCaseDigits)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"FF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenMixedCaseDigits)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"fF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenLowerCasePrefixed)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"0xFF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenUpperCasePrefixed)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"0XFF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ThrowsWhenInputNotANumber)
{
	EXPECT_THROW((Util::ParseNumber<uint32_t, 16>(L"foobar")), std::invalid_argument);
}