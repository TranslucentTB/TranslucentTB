#include <cstdint>
#include <gtest/gtest.h>

#include "util/numbers.hpp"

TEST(ClampTo, UnsignedToSignedClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<int16_t, uint32_t>(4000000)), 32767);
}

TEST(ClampTo, UnsignedToSignedDoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<int16_t, uint32_t>(300)), 300);
}

TEST(ClampTo, UnsignedToUnsignedClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, uint32_t>(4000000)), 65535);
}

TEST(ClampTo, UnsignedToUnsignedDoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, uint32_t>(300)), 300);
}

TEST(ClampTo, SignedToUnsignedClampsToZeroWhenSourceNegative)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, int32_t>(-2000000)), 0);
}

TEST(ClampTo, SignedToUnsignedClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, int32_t>(2000000)), 65535);
}

TEST(ClampTo, SignedToUnsignedDoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<uint16_t, int32_t>(300)), 300);
}

TEST(ClampTo, SignedToSignedClampsToMinimumValueWhenSourceTooSmall)
{
	EXPECT_EQ((Util::ClampTo<int16_t, int32_t>(-2000000)), -32768);
}

TEST(ClampTo, SignedToSignedClampsToMaximumValueWhenSourceTooBig)
{
	EXPECT_EQ((Util::ClampTo<int16_t, int32_t>(2000000)), 32767);
}

TEST(ClampTo, SignedToSignedDoesNotClampsWhenSourceFits)
{
	EXPECT_EQ((Util::ClampTo<int16_t, int32_t>(300)), 300);
}

TEST(IsDecimalDigit, ReturnsFalseWhenNotDigit)
{
	EXPECT_FALSE(Util::impl::IsDecimalDigit(L'a'));
}

TEST(IsDecimalDigit, ReturnsTrueWhenDigit)
{
	EXPECT_TRUE(Util::impl::IsDecimalDigit(L'5'));
}

TEST(IsCapitalHexDigit, ReturnsFalseWhenNotDigit)
{
	EXPECT_FALSE(Util::impl::IsCapitalHexDigit(L'R'));
}

TEST(IsCapitalHexDigit, ReturnsFalseWhenLowerCaseDigit)
{
	EXPECT_FALSE(Util::impl::IsCapitalHexDigit(L'f'));
}

TEST(IsCapitalHexDigit, ReturnsTrueWhenUpperCaseDigit)
{
	EXPECT_TRUE(Util::impl::IsCapitalHexDigit(L'F'));
}

TEST(IsLowerHexDigit, ReturnsFalseWhenNotDigit)
{
	EXPECT_FALSE(Util::impl::IsLowerHexDigit(L'r'));
}

TEST(IsLowerHexDigit, ReturnsFalseWhenUpperCaseDigit)
{
	EXPECT_FALSE(Util::impl::IsLowerHexDigit(L'F'));
}

TEST(IsLowerHexDigit, ReturnsTrueWhenLowerCaseDigit)
{
	EXPECT_TRUE(Util::impl::IsLowerHexDigit(L'f'));
}

TEST(pow, ReturnsOneWhenPowerIsZero)
{
	EXPECT_EQ(Util::impl::pow<uint64_t>(164, 0), 1);
}

TEST(pow, ReturnsCorrectPowerOfTen)
{
	EXPECT_EQ(Util::impl::pow<uint64_t>(10, 6), 1000000);
}

TEST(ParseNumberUnsignedBaseTen, ThrowsWhenInputNegative)
{
	EXPECT_THROW((Util::ParseNumber<uint32_t, 10>(L"-10")), std::out_of_range);
}

TEST(ParseNumberUnsignedBaseTen, ReturnsCorrectValueWhenInputPositive)
{
	EXPECT_EQ((Util::ParseNumber<uint32_t, 10>(L"1000")), 1000);
}

TEST(ParseNumberSignedBaseTen, ReturnsCorrectValueWhenInputNegative)
{
	EXPECT_EQ((Util::ParseNumber<int32_t, 10>(L"-1000")), -1000);
}

TEST(ParseNumberSignedBaseTen, ReturnsCorrectValueWhenInputPositive)
{
	EXPECT_EQ((Util::ParseNumber<int32_t, 10>(L"1000")), 1000);
}

TEST(ParseNumberBaseTen, ThrowsWhenInputNotANumber)
{
	EXPECT_THROW((Util::ParseNumber<int32_t, 10>(L"foobar")), std::invalid_argument);
}

TEST(ParseNumberBaseSixteen, ReturnsCorrectValueWhenLowerCaseDigits)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"ff")), 255);
}

TEST(ParseNumberBaseSixteen, ReturnsCorrectValueWhenUpperCaseDigits)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"FF")), 255);
}

TEST(ParseNumberBaseSixteen, ReturnsCorrectValueWhenMixedCaseDigits)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"fF")), 255);
}

TEST(ParseNumberBaseSixteen, ReturnsCorrectValueWhenLowerCasePrefixed)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"0xFF")), 255);
}

TEST(ParseNumberBaseSixteen, ReturnsCorrectValueWhenUpperCasePrefixed)
{
	EXPECT_EQ((Util::ParseNumber<uint8_t, 16>(L"0XFF")), 255);
}

TEST(ParseNumberBaseSixteen, ThrowsWhenInputNotANumber)
{
	EXPECT_THROW((Util::ParseNumber<uint32_t, 16>(L"foobar")), std::invalid_argument);
}