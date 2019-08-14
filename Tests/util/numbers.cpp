#include <cstdint>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "util/numbers.hpp"

using namespace testing;

TEST(Util_IsDecimalDigit, ReturnsFalseWhenNotDigit)
{
	ASSERT_THAT(Util::impl::IsDecimalDigit(L'a'), IsFalse());
}

TEST(Util_IsDecimalDigit, ReturnsTrueWhenDigit)
{
	ASSERT_THAT(Util::impl::IsDecimalDigit(L'5'), IsTrue());
}

TEST(Util_IsUpperHexDigit, ReturnsFalseWhenNotDigit)
{
	ASSERT_THAT(Util::impl::IsUpperHexDigit(L'R'), IsFalse());
}

TEST(Util_IsUpperHexDigit, ReturnsFalseWhenLowerCaseDigit)
{
	ASSERT_THAT(Util::impl::IsUpperHexDigit(L'f'), IsFalse());
}

TEST(Util_IsUpperHexDigit, ReturnsTrueWhenUpperCaseDigit)
{
	ASSERT_THAT(Util::impl::IsUpperHexDigit(L'F'), IsTrue());
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
	ASSERT_THROW(Util::ParseNumber<uint32_t>(L"-10"), std::out_of_range);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, ThrowsOnTooManyDigits)
{
	ASSERT_THROW(Util::ParseNumber<int8_t>(L"1000"), std::out_of_range);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, ReturnsCorrectValue)
{
	ASSERT_EQ(Util::ParseNumber<uint32_t>(L"1000"), 1000);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, HandlesVeryLargeNumber)
{
	ASSERT_EQ(Util::ParseNumber<uint64_t>(L"18446744073709551610"), 18446744073709551610);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, HandlesMaximumValue)
{
	ASSERT_EQ(Util::ParseNumber<uint8_t>(L"255"), 255);
}

TEST(Util_ParseNumber_Unsigned_BaseTen, HandlesMinimumValue)
{
	ASSERT_EQ(Util::ParseNumber<uint8_t>(L"0"), 0);
}

TEST(Util_ParseNumber_Signed_BaseTen, ThrowsOnTooManyDigits)
{
	ASSERT_THROW(Util::ParseNumber<int8_t>(L"-1000"), std::out_of_range);
}

TEST(Util_ParseNumber_Signed_BaseTen, ReturnsCorrectValueWhenInputNegative)
{
	ASSERT_EQ(Util::ParseNumber<int32_t>(L"-1000"), -1000);
}

TEST(Util_ParseNumber_Signed_BaseTen, ReturnsCorrectValueWhenInputPositive)
{
	ASSERT_EQ(Util::ParseNumber<int32_t>(L"1000"), 1000);
}

TEST(Util_ParseNumber_Signed_BaseTen, HandlesVeryLargeNumber)
{
	ASSERT_EQ(Util::ParseNumber<int64_t>(L"9223372036854775800"), 9223372036854775800);
}

TEST(Util_ParseNumber_Signed_BaseTen, HandlesVeryLargeNegativeNumber)
{
	ASSERT_EQ(Util::ParseNumber<int64_t>(L"-9223372036854775800"), -9223372036854775800);
}

TEST(Util_ParseNumber_Signed_BaseTen, HandlesMaximumValue)
{
	ASSERT_EQ(Util::ParseNumber<int8_t>(L"127"), 127);
}

TEST(Util_ParseNumber_Signed_BaseTen, HandlesMinimumValue)
{
	ASSERT_EQ(Util::ParseNumber<int8_t>(L"-128"), -128);
}

TEST(Util_ParseNumber_BaseTen, ThrowsWhenInputNotANumber)
{
	ASSERT_THROW(Util::ParseNumber<int32_t>(L"foobar"), std::invalid_argument);
}

TEST(Util_ParseNumber_BaseSixteen, ThrowsWhenInputNotANumber)
{
	ASSERT_THROW((Util::ParseNumber<uint32_t, 16>(L"foobar")), std::invalid_argument);
}

TEST(Util_ParseNumber_BaseSixteen, ThrowsOnTooManyDigits)
{
	ASSERT_THROW((Util::ParseNumber<uint8_t, 16>(L"100")), std::out_of_range);
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

TEST(Util_ParseNumber_BaseSixteen, HandlesVeryLargeNumber)
{
	ASSERT_EQ((Util::ParseNumber<uint64_t, 16>(L"0xFFFFFFFFFFFFFFFA")), 0xFFFFFFFFFFFFFFFA);
}

TEST(Util_ParseNumber_BaseSixteen, HandlesMaximumValue)
{
	ASSERT_EQ((Util::ParseNumber<uint64_t, 16>(L"0xFFFFFFFFFFFFFFFF")), 0xFFFFFFFFFFFFFFFF);
}

TEST(Util_ParseNumber_BaseSixteen, HandlesMinimumValue)
{
	ASSERT_EQ((Util::ParseNumber<uint64_t, 16>(L"0x0")), 0);
}

TEST(Util_ParseNumber, TrimsInput)
{
	ASSERT_EQ(Util::ParseNumber(L" \t \n 10  \r "), 10);
}

TEST(Util_ExpandOneHexDigitByte, ExpandsByte)
{
	ASSERT_EQ(Util::ExpandOneHexDigitByte(0xF), 0xFF);
}

TEST(Util_ExpandOneHexDigitByte, IgnoresSecondDigit)
{
	ASSERT_EQ(Util::ExpandOneHexDigitByte(0xAF), 0xFF);
}
