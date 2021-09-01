#include <gtest/gtest.h>
#include <ranges>

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
	for (const auto number : numbers)
	{
		ASSERT_TRUE(Util::IsAscii(number));
	}

	for (const auto [upper, lower] : alphabet)
	{
		ASSERT_TRUE(Util::IsAscii(upper));
		ASSERT_TRUE(Util::IsAscii(lower));
	}

	for (const auto character : specialCharacters)
	{
		ASSERT_TRUE(Util::IsAscii(character));
	}
}

TEST(Util_IsAscii, ReturnsFalseWhenNotAscii)
{
	ASSERT_FALSE(Util::IsAscii(L'\u00CB'));
	ASSERT_FALSE(Util::IsAscii(L'\u0125'));
}

TEST(Util_AsciiToUpper, ReturnsUppercaseAsciiFromLowercase)
{
	for (const auto [upper, lower] : alphabet)
	{
		ASSERT_EQ(Util::AsciiToUpper(lower), upper);
	}
}

TEST(Util_AsciiToUpper, ReturnsUppercaseAsciiFromUppercase)
{
	for (const auto letter : alphabet | std::views::elements<0>)
	{
		ASSERT_EQ(Util::AsciiToUpper(letter), letter);
	}
}

TEST(Util_AsciiToUpper, DoesNotChangesOtherCharacters)
{
	for (const auto number : numbers)
	{
		ASSERT_EQ(Util::AsciiToUpper(number), number);
	}

	for (const auto character : specialCharacters)
	{
		ASSERT_EQ(Util::AsciiToUpper(character), character);
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
