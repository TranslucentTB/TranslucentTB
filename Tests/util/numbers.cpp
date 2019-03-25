#include <cstdint>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "util/numbers.hpp"

using namespace testing;

TEST(Util_ClampTo_Unsigned_Signed, ClampsToMaximumValueWhenSourceTooBig)
{
	ASSERT_EQ((Util::ClampTo<int16_t, uint32_t>(4000000)), 32767);
}

TEST(Util_ClampTo_Unsigned_Signed, DoesNotClampsWhenSourceFits)
{
	ASSERT_EQ((Util::ClampTo<int16_t, uint32_t>(300)), 300);
}

TEST(Util_ClampTo_Unsigned_Unsigned, ClampsToMaximumValueWhenSourceTooBig)
{
	ASSERT_EQ((Util::ClampTo<uint16_t, uint32_t>(4000000)), 65535);
}

TEST(Util_ClampTo_Unsigned_Unsigned, DoesNotClampsWhenSourceFits)
{
	ASSERT_EQ((Util::ClampTo<uint16_t, uint32_t>(300)), 300);
}

TEST(Util_ClampTo_Signed_Unsigned, ClampsToZeroWhenSourceNegative)
{
	ASSERT_EQ((Util::ClampTo<uint16_t, int32_t>(-2000000)), 0);
}

TEST(Util_ClampTo_Signed_Unsigned, ClampsToMaximumValueWhenSourceTooBig)
{
	ASSERT_EQ((Util::ClampTo<uint16_t, int32_t>(2000000)), 65535);
}

TEST(Util_ClampTo_Signed_Unsigned, DoesNotClampsWhenSourceFits)
{
	ASSERT_EQ((Util::ClampTo<uint16_t, int32_t>(300)), 300);
}

TEST(Util_ClampTo_Signed_Signed, ClampsToMinimumValueWhenSourceTooSmall)
{
	ASSERT_EQ((Util::ClampTo<int16_t, int32_t>(-2000000)), -32768);
}

TEST(Util_ClampTo_Signed_Signed, ClampsToMaximumValueWhenSourceTooBig)
{
	ASSERT_EQ((Util::ClampTo<int16_t, int32_t>(2000000)), 32767);
}

TEST(Util_ClampTo_Signed_Signed, DoesNotClampsWhenSourceFits)
{
	ASSERT_EQ((Util::ClampTo<int16_t, int32_t>(300)), 300);
}

TEST(Util_IsDecimalDigit, ReturnsFalseWhenNotDigit)
{
	ASSERT_THAT(Util::impl::IsDecimalDigit(L'a'), IsFalse());
}

TEST(Util_IsDecimalDigit, ReturnsTrueWhenDigit)
{
	ASSERT_THAT(Util::impl::IsDecimalDigit(L'5'), IsTrue());
}

TEST(Util_IsCapitalHexDigit, ReturnsFalseWhenNotDigit)
{
	ASSERT_THAT(Util::impl::IsCapitalHexDigit(L'R'), IsFalse());
}

TEST(Util_IsCapitalHexDigit, ReturnsFalseWhenLowerCaseDigit)
{
	ASSERT_THAT(Util::impl::IsCapitalHexDigit(L'f'), IsFalse());
}

TEST(Util_IsCapitalHexDigit, ReturnsTrueWhenUpperCaseDigit)
{
	ASSERT_THAT(Util::impl::IsCapitalHexDigit(L'F'), IsTrue());
}

TEST(Util_IsLowerHexDigit, ReturnsFalseWhenNotDigit)
{
	ASSERT_THAT(Util::impl::IsLowerHexDigit(L'r'), IsFalse());
}

TEST(Util_IsLowerHexDigit, ReturnsFalseWhenUpperCaseDigit)
{
	ASSERT_THAT(Util::impl::IsLowerHexDigit(L'F'), IsFalse());
}

TEST(Util_IsLowerHexDigit, ReturnsTrueWhenLowerCaseDigit)
{
	ASSERT_THAT(Util::impl::IsLowerHexDigit(L'f'), IsTrue());
}

TEST(Util_pow, ReturnsOneWhenPowerIsZero)
{
	ASSERT_EQ(Util::impl::pow<uint64_t>(164, 0), 1);
}

TEST(Util_pow, ReturnsCorrectPowerOfTen)
{
	ASSERT_EQ(Util::impl::pow<uint64_t>(10, 6), 1000000);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, ThrowsWhenInputNegative)
{
	ASSERT_THROW((Util::ParseNumber<uint32_t, 10>(L"-10")), std::out_of_range);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, ReturnsCorrectValueWhenInputPositive)
{
	ASSERT_EQ((Util::ParseNumber<uint32_t, 10>(L"1000")), 1000);
}

TEST(Util_ParseNumber_Signed_BaseTen, ReturnsCorrectValueWhenInputNegative)
{
	ASSERT_EQ((Util::ParseNumber<int32_t, 10>(L"-1000")), -1000);
}

TEST(Util_ParseNumber_Signed_BaseTen, ReturnsCorrectValueWhenInputPositive)
{
	ASSERT_EQ((Util::ParseNumber<int32_t, 10>(L"1000")), 1000);
}

TEST(Util_ParseNumber_BaseTen, ThrowsWhenInputNotANumber)
{
	ASSERT_THROW((Util::ParseNumber<int32_t, 10>(L"foobar")), std::invalid_argument);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenLowerCaseDigits)
{
	ASSERT_EQ((Util::ParseNumber<uint8_t, 16>(L"ff")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenUpperCaseDigits)
{
	ASSERT_EQ((Util::ParseNumber<uint8_t, 16>(L"FF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenMixedCaseDigits)
{
	ASSERT_EQ((Util::ParseNumber<uint8_t, 16>(L"fF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenLowerCasePrefixed)
{
	ASSERT_EQ((Util::ParseNumber<uint8_t, 16>(L"0xFF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ReturnsCorrectValueWhenUpperCasePrefixed)
{
	ASSERT_EQ((Util::ParseNumber<uint8_t, 16>(L"0XFF")), 255);
}

TEST(Util_ParseNumber_BaseSixteen, ThrowsWhenInputNotANumber)
{
	ASSERT_THROW((Util::ParseNumber<uint32_t, 16>(L"foobar")), std::invalid_argument);
}