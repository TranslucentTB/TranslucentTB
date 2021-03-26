#include <cstdint>
#include <gtest/gtest.h>
#include <ranges>

#include "../testingdata.hpp"
#include "util/numbers.hpp"

TEST(Util_IsDecimalDigit, ReturnsFalseWhenNotDigit)
{
	for (const auto [upper, lower] : alphabet)
	{
		ASSERT_FALSE(Util::impl::IsDecimalDigit(upper));
		ASSERT_FALSE(Util::impl::IsDecimalDigit(lower));
	}

	for (const auto character : specialCharacters)
	{
		ASSERT_FALSE(Util::impl::IsDecimalDigit(character));
	}
}

TEST(Util_IsDecimalDigit, ReturnsTrueWhenDigit)
{
	for (const auto number : numbers)
	{
		ASSERT_TRUE(Util::impl::IsDecimalDigit(number));
	}
}

TEST(Util_IsUpperHexDigit, ReturnsFalseWhenNotUpperCaseDigit)
{
	for (const auto number : numbers)
	{
		ASSERT_FALSE(Util::impl::IsUpperHexDigit(number));
	}

	for (const auto letter : alphabet | std::views::elements<0> | std::views::drop(6))
	{
		ASSERT_FALSE(Util::impl::IsUpperHexDigit(letter));
	}

	for (const auto letter : alphabet | std::views::elements<1>)
	{
		ASSERT_FALSE(Util::impl::IsUpperHexDigit(letter));
	}

	for (const auto character : specialCharacters)
	{
		ASSERT_FALSE(Util::impl::IsUpperHexDigit(character));
	}
}

TEST(Util_IsUpperHexDigit, ReturnsTrueWhenUpperCaseDigit)
{
	for (const auto letter : alphabet | std::views::elements<0> | std::views::take(6))
	{
		ASSERT_TRUE(Util::impl::IsUpperHexDigit(letter));
	}
}

TEST(Util_IsLowerHexDigit, ReturnsFalseWhenNotLowerCaseDigit)
{
	for (const auto number : numbers)
	{
		ASSERT_FALSE(Util::impl::IsLowerHexDigit(number));
	}

	for (const auto letter : alphabet | std::views::elements<0>)
	{
		ASSERT_FALSE(Util::impl::IsLowerHexDigit(letter));
	}

	for (const auto letter : alphabet | std::views::elements<1> | std::views::drop(6))
	{
		ASSERT_FALSE(Util::impl::IsLowerHexDigit(letter));
	}

	for (const auto character : specialCharacters)
	{
		ASSERT_FALSE(Util::impl::IsLowerHexDigit(character));
	}
}

TEST(Util_IsLowerHexDigit, ReturnsTrueWhenLowerCaseDigit)
{
	for (const auto letter : alphabet | std::views::elements<1> | std::views::take(6))
	{
		ASSERT_TRUE(Util::impl::IsLowerHexDigit(letter));
	}
}

TEST(Util_ParseHexNumber, ThrowsWhenInputNotANumber)
{
	static constexpr std::wstring_view testCases[] = {
		L"foobar",
		L"0x",
		L"",
		L"  \n \t\r"
	};

	for (const auto &testCase : testCases)
	{
		ASSERT_THROW(Util::ParseHexNumber(testCase), std::invalid_argument);
	}
}

TEST(Util_ParseHexNumber, ThrowsOnOverflow)
{
	ASSERT_THROW(Util::ParseHexNumber<uint8_t>(L"100"), std::out_of_range);
}

TEST(Util_ParseHexNumber, ReturnsCorrectValue)
{
	static constexpr std::pair<std::wstring_view, uint64_t> cases[] = {
		{ L"af", 0xAF },
		{ L"AF", 0xAF },
		{ L"aF", 0xAF },
		{ L"0xAF", 0xAF },
		{ L"0XAF", 0xAF },
		{ L"0xFFFFFFFFFFFFFFFA", 0xFFFFFFFFFFFFFFFA },
		{ L"0xFFFFFFFFFFFFFFFF", 0xFFFFFFFFFFFFFFFF },
		{ L"0x0", 0x0 },
		{ L"A", 0xA },
		{ L" \t \n 10  \r ", 0x10 }
	};

	for (const auto &testCase : cases)
	{
		ASSERT_EQ(Util::ParseHexNumber<uint64_t>(testCase.first), testCase.second);
	}
}

TEST(Util_ExpandOneHexDigitByte, ExpandsByte)
{
	ASSERT_EQ(Util::ExpandOneHexDigitByte(0xF), 0xFF);
}

TEST(Util_ExpandOneHexDigitByte, IgnoresSecondDigit)
{
	ASSERT_EQ(Util::ExpandOneHexDigitByte(0xAF), 0xFF);
}
