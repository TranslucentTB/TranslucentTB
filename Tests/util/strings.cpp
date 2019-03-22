#include <gtest/gtest.h>

#include "util/strings.hpp"

TEST(Util_ToLowerInplace, TurnsStringLowercase)
{
	std::wstring str = L"FOO";
	Util::ToLowerInplace(str);
	EXPECT_EQ(str, L"foo");
}

TEST(Util_ToLower, ReturnsLowercaseString)
{
	EXPECT_EQ(Util::ToLower(L"FOO"), L"foo");
}

TEST(Util_Equals, ReturnsFalseWhenLengthDifferent)
{
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"foo", L"foobar"));
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"FOOBAR", L"FOO"));
}

TEST(Util_Equals, ReturnsFalseWhenContentDifferent)
{
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"foo", L"bar"));
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"FOO", L"BAR"));
}

TEST(Util_Equals, ReturnsTrueWhenCaseDifferent)
{
	EXPECT_TRUE(Util::IgnoreCaseStringEquals(L"foo", L"FOO"));
	EXPECT_TRUE(Util::IgnoreCaseStringEquals(L"FOOBAR", L"foobar"));
}

TEST(Util_Equals, ReturnsTrueWhenContentSame)
{
	EXPECT_TRUE(Util::IgnoreCaseStringEquals(L"foo", L"foo"));
}

TEST(Util_StringLowerCaseHash, DifferentWhenLengthDifferent)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_NE(hasher(L"foo"), hasher(L"foobar"));
	EXPECT_NE(hasher(L"FOOBAR"),hasher(L"FOO"));
}

TEST(Util_StringLowerCaseHash, DifferentWhenContentDifferent)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_NE(hasher(L"foo"), hasher(L"bar"));
	EXPECT_NE(hasher(L"FOO"), hasher(L"BAR"));
}

TEST(Util_StringLowerCaseHash, SameWhenCaseDifferent)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_EQ(hasher(L"foo"), hasher(L"FOO"));
	EXPECT_EQ(hasher(L"FOOBAR"), hasher(L"foobar"));
}

TEST(Util_StringLowerCaseHash, SameWhenContentSame)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_EQ(hasher(L"foo"), hasher(L"foo"));
}

TEST(Util_Trim, TrimsLeft)
{
	EXPECT_EQ(Util::Trim(L"\t\v \f\n\rfoo \nbar"), L"foo \nbar");
}

TEST(Util_Trim, TrimsRight)
{
	EXPECT_EQ(Util::Trim(L"foo \nbar\t\r \f\n\v"), L"foo \nbar");
}


TEST(Util_Trim, TrimsLeftRight)
{
	EXPECT_EQ(Util::Trim(L"\t\v \f\n\rfoo \nbar\t\r \f\n\v"), L"foo \nbar");
}

TEST(Util_Trim, TrimsNothing)
{
	EXPECT_EQ(Util::Trim(L"foo \nbar"), L"foo \nbar");
}

TEST(Util_Trim, TrimsAll)
{
	EXPECT_EQ(Util::Trim(L" \f\n\r\t\v"), std::wstring_view{});
}

TEST(Util_TrimInplace_StringView, TrimsLeft)
{
	std::wstring_view str = L"\t\v \f\n\rfoo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsRight)
{
	std::wstring_view str = L"foo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsLeftRight)
{
	std::wstring_view str = L"\t\v \f\n\rfoo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsNothing)
{
	std::wstring_view str = L"foo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsAll)
{
	std::wstring_view str = L" \f\n\r\t\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, std::wstring_view{});
}

TEST(Util_TrimInplace_String, TrimsLeft)
{
	std::wstring str = L"\t\v \f\n\rfoo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsRight)
{
	std::wstring str = L"foo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsLeftRight)
{
	std::wstring str = L"\t\v \f\n\rfoo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsNothing)
{
	std::wstring str = L"foo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsAll)
{
	std::wstring str = L" \f\n\r\t\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, std::wstring{});
}

TEST(Util_StringBeginsWith, ReturnsFalseWhenStringShorterThanPrefix)
{
	EXPECT_FALSE(Util::StringBeginsWith(L"foo", L"foobar"));
}

TEST(Util_StringBeginsWith, ReturnsFalseWhenStringDoesNotBeginsWith)
{
	EXPECT_FALSE(Util::StringBeginsWith(L"foobar", L"bar"));
}

TEST(Util_StringBeginsWith, ReturnsTrueWhenSame)
{
	EXPECT_TRUE(Util::StringBeginsWith(L"foobar", L"foobar"));
}

TEST(Util_StringBeginsWith, ReturnsTrueWhenStringLongerThanPrefixAndBeginsWith)
{
	EXPECT_TRUE(Util::StringBeginsWith(L"foobar", L"foo"));
}

TEST(Util_StringBeginsWithOneOf, ReturnsTrueWhenStringBeginsWithOneOf)
{
	EXPECT_TRUE(Util::StringBeginsWithOneOf(L"foobar", { L"bar", L"foobar", L"foo" }));
}

TEST(Util_StringBeginsWithOneOf, ReturnsFalseWhenStringDoesNotBeginsWithOneOf)
{
	EXPECT_FALSE(Util::StringBeginsWithOneOf(L"buz", { L"bar", L"foobar", L"foo" }));
}

TEST(Util_RemovePrefix, ReturnsPrefixlessStringWhenPrefixPresent)
{
	EXPECT_EQ(Util::RemovePrefix(L"foobar", L"foo"), L"bar");
}

TEST(Util_RemovePrefix, ReturnsSameStringWhenPrefixAbsent)
{
	EXPECT_EQ(Util::RemovePrefix(L"foo", L"bar"), L"foo");
}

TEST(Util_RemovePrefix, ReturnsNullWhenInputEqualsPrefix)
{
	EXPECT_EQ(Util::RemovePrefix(L"foo", L"foo"), std::wstring_view{});
}

TEST(Util_RemovePrefixInplace_StringView, RemovesPrefixWhenPrefixPresent)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"foo");
	EXPECT_EQ(str, L"bar");
}

TEST(Util_RemovePrefixInplace_StringView, DoesNotChangesVariableWhenPrefixAbsent)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"bar");
	EXPECT_EQ(str, L"foobar");
}

TEST(Util_RemovePrefixInplace_StringView, ChangesVariableToNullWhenInputEqualsPrefix)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"foobar");
	EXPECT_EQ(str, std::wstring_view{});
}

TEST(Util_RemovePrefixInplace_String, RemovesPrefixWhenPrefixPresent)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"foo");
	EXPECT_EQ(str, L"bar");
}

TEST(Util_RemovePrefixInplace_String, DoesNotChangesVariableWhenPrefixAbsent)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"bar");
	EXPECT_EQ(str, L"foobar");
}

TEST(Util_RemovePrefixInplace_String, ChangesVariableToNullWhenInputEqualsPrefix)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"foobar");
	EXPECT_EQ(str, std::wstring{});
}