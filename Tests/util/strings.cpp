#include <gtest/gtest.h>

#include "../testingdata.hpp"
#include "util/strings.hpp"

namespace {
	static constexpr std::pair<std::wstring_view, std::wstring_view> trimTestCases[] = {
		{ L"\t\v \f\n\rfoo \nbar", L"foo \nbar" },
		{ L"foo \nbar\t\r \f\n\v", L"foo \nbar" },
		{ L"\t\v \f\n\rfoo \nbar\t\r \f\n\v", L"foo \nbar" },
		{ L"foo \nbar", L"foo \nbar" },
		{ L" \f\n\r\t\v", L"" }
	};
}

TEST(Util_IsAscii, ReturnsTrueWhenAscii)
{
	for (const auto &number : numbers)
	{
		ASSERT_TRUE(Util::IsAscii(number));
	}

	for (const auto &letter : uppercaseAlphabet)
	{
		ASSERT_TRUE(Util::IsAscii(letter));
	}

	for (const auto &letter : lowercaseAlphabet)
	{
		ASSERT_TRUE(Util::IsAscii(letter));
	}
}

TEST(Util_IsAscii, ReturnsFalseWhenNotAscii)
{
	ASSERT_FALSE(Util::IsAscii(L'\u00CB'));
}

TEST(Util_AsciiToUpper, ReturnsUppercaseAsciiFromLowercase)
{
	static_assert(std::size(lowercaseAlphabet) == std::size(uppercaseAlphabet));

	std::pair<wchar_t, wchar_t> alphabet[std::size(lowercaseAlphabet)];
	for (std::size_t i = 0; i < std::size(lowercaseAlphabet); ++i)
	{
		alphabet[i] = { lowercaseAlphabet[i], uppercaseAlphabet[i] };
	}

	for (const auto &[lower, upper] : alphabet)
	{
		ASSERT_EQ(Util::AsciiToUpper(lower), upper);
	}
}

TEST(Util_AsciiToUpper, ReturnsUppercaseAsciiFromUppercase)
{
	for (const auto &letter : uppercaseAlphabet)
	{
		ASSERT_EQ(Util::AsciiToUpper(letter), letter);
	}
}

TEST(Util_Trim, Trims)
{
	for (const auto &[input, expected] : trimTestCases)
	{
		ASSERT_EQ(Util::Trim(input), expected);
	}
}

TEST(Util_TrimInplace_StringView, Trims)
{
	for (auto [input, expected] : trimTestCases)
	{
		Util::TrimInplace(input);
		ASSERT_EQ(input, expected);
	}
}

TEST(Util_TrimInplace_String, Trims)
{
	for (const auto &[input, expected] : trimTestCases)
	{
		std::wstring str(input);
		Util::TrimInplace(str);
		ASSERT_EQ(str, expected);
	}
}
