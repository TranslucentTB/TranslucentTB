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

TEST(Util_ParseHexNumber, ThrowsWhenInputNotANumber)
{
	ASSERT_THROW(Util::ParseHexNumber(L"foobar"), std::invalid_argument);
}

TEST(Util_ParseHexNumber, ThrowsOnOverflow)
{
	ASSERT_THROW(Util::ParseHexNumber<uint8_t>(L"100"), std::out_of_range);
}

TEST(Util_ParseHexNumber, ReturnsCorrectValueWhenLowerCaseDigits)
{
	ASSERT_EQ(Util::ParseHexNumber<uint8_t>(L"af"), 0xAF);
}

TEST(Util_ParseHexNumber, ReturnsCorrectValueWhenUpperCaseDigits)
{
	ASSERT_EQ(Util::ParseHexNumber<uint8_t>(L"AF"), 0xAF);
}

TEST(Util_ParseHexNumber, ReturnsCorrectValueWhenMixedCaseDigits)
{
	ASSERT_EQ(Util::ParseHexNumber<uint8_t>(L"aF"), 0xAF);
}

TEST(Util_ParseHexNumber, ReturnsCorrectValueWhenLowerCasePrefixed)
{
	ASSERT_EQ(Util::ParseHexNumber<uint8_t>(L"0xAF"), 0xAF);
}

TEST(Util_ParseHexNumber, ReturnsCorrectValueWhenUpperCasePrefixed)
{
	ASSERT_EQ(Util::ParseHexNumber<uint8_t>(L"0XAF"), 0xAF);
}

TEST(Util_ParseHexNumber, HandlesVeryLargeNumber)
{
	ASSERT_EQ(Util::ParseHexNumber<uint64_t>(L"0xFFFFFFFFFFFFFFFA"), 0xFFFFFFFFFFFFFFFA);
}

TEST(Util_ParseHexNumber, HandlesMaximumValue)
{
	ASSERT_EQ(Util::ParseHexNumber<uint64_t>(L"0xFFFFFFFFFFFFFFFF"), 0xFFFFFFFFFFFFFFFF);
}

TEST(Util_ParseHexNumber, HandlesMinimumValue)
{
	ASSERT_EQ(Util::ParseHexNumber<uint64_t>(L"0x0"), 0x0);
}

TEST(Util_ParseHexNumber, HandlesOneDigit)
{
	ASSERT_EQ(Util::ParseHexNumber<uint8_t>(L"A"), 0xA);
}

TEST(Util_ParseHexNumber, TrimsInput)
{
	ASSERT_EQ(Util::ParseHexNumber(L" \t \n 10  \r "), 0x10);
}

TEST(Util_ExpandOneHexDigitByte, ExpandsByte)
{
	ASSERT_EQ(Util::ExpandOneHexDigitByte(0xF), 0xFF);
}

TEST(Util_ExpandOneHexDigitByte, IgnoresSecondDigit)
{
	ASSERT_EQ(Util::ExpandOneHexDigitByte(0xAF), 0xFF);
}
