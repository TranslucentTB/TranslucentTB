#include <cstdint>
#include <gtest/gtest.h>
#include <span>

#include "util/numbers.hpp"

namespace {
	static constexpr wchar_t numbers[] = {
		L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9'
	};

	static constexpr wchar_t uppercaseAlphabet[] = {
		L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H', L'I',
		L'J', L'K', L'L', L'M', L'N', L'O', L'P', L'Q', L'R',
		L'S', L'T', L'U', L'V', L'W', L'X', L'Y', L'Z'
	};

	static constexpr wchar_t lowercaseAlphabet[] = {
		L'a', L'b', L'c', L'd', L'e', L'f', L'g', L'h', L'i',
		L'j', L'k', L'l', L'm', L'n', L'o', L'p', L'q', L'r',
		L's', L't', L'u', L'v', L'w', L'x', L'y', L'z'
	};
}

TEST(Util_IsDecimalDigit, ReturnsFalseWhenNotDigit)
{
	for (const auto &letter : uppercaseAlphabet)
	{
		ASSERT_FALSE(Util::impl::IsDecimalDigit(letter));
	}

	for (const auto &letter : lowercaseAlphabet)
	{
		ASSERT_FALSE(Util::impl::IsDecimalDigit(letter));
	}
}

TEST(Util_IsDecimalDigit, ReturnsTrueWhenDigit)
{
	for (const auto &number : numbers)
	{
		ASSERT_TRUE(Util::impl::IsDecimalDigit(number));
	}
}

TEST(Util_IsUpperHexDigit, ReturnsFalseWhenNotUpperCaseDigit)
{
	for (const auto &number : numbers)
	{
		ASSERT_FALSE(Util::impl::IsUpperHexDigit(number));
	}

	for (const auto &letter : std::span(uppercaseAlphabet).subspan(6))
	{
		ASSERT_FALSE(Util::impl::IsUpperHexDigit(letter));
	}

	for (const auto &letter : lowercaseAlphabet)
	{
		ASSERT_FALSE(Util::impl::IsUpperHexDigit(letter));
	}
}

TEST(Util_IsUpperHexDigit, ReturnsTrueWhenUpperCaseDigit)
{
	for (const auto &letter : std::span(uppercaseAlphabet).subspan(0, 6))
	{
		ASSERT_TRUE(Util::impl::IsUpperHexDigit(letter));
	}
}

TEST(Util_IsLowerHexDigit, ReturnsFalseWhenNotLowerCaseDigit)
{
	for (const auto &number : numbers)
	{
		ASSERT_FALSE(Util::impl::IsLowerHexDigit(number));
	}

	for (const auto &letter : uppercaseAlphabet)
	{
		ASSERT_FALSE(Util::impl::IsLowerHexDigit(letter));
	}

	for (const auto &letter : std::span(lowercaseAlphabet).subspan(6))
	{
		ASSERT_FALSE(Util::impl::IsLowerHexDigit(letter));
	}
}

TEST(Util_IsLowerHexDigit, ReturnsTrueWhenLowerCaseDigit)
{
	for (const auto &letter : std::span(lowercaseAlphabet).subspan(0, 6))
	{
		ASSERT_TRUE(Util::impl::IsLowerHexDigit(letter));
	}
}

TEST(Util_ParseHexNumber, ThrowsWhenInputNotANumber)
{
	ASSERT_THROW(Util::ParseHexNumber(L"foobar"), std::invalid_argument);
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
