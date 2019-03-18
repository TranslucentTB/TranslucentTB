#include <gtest/gtest.h>

#include "util/strings.hpp"

TEST(ToLowerInplace, TurnsStringLowercase)
{
	std::wstring str = L"FOO";
	Util::ToLowerInplace(str);
	EXPECT_EQ(str, L"foo");
}

TEST(ToLower, ReturnsLowercaseString)
{
	EXPECT_EQ(Util::ToLower(L"FOO"), L"foo");
}

TEST(Equals, ReturnsFalseWhenLengthDifferent)
{
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"foo", L"foobar"));
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"FOOBAR", L"FOO"));
}

TEST(Equals, ReturnsFalseWhenContentDifferent)
{
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"foo", L"bar"));
	EXPECT_FALSE(Util::IgnoreCaseStringEquals(L"FOO", L"BAR"));
}

TEST(Equals, ReturnsTrueWhenCaseDifferent)
{
	EXPECT_TRUE(Util::IgnoreCaseStringEquals(L"foo", L"FOO"));
	EXPECT_TRUE(Util::IgnoreCaseStringEquals(L"FOOBAR", L"foobar"));
}

TEST(Equals, ReturnsTrueWhenContentSame)
{
	EXPECT_TRUE(Util::IgnoreCaseStringEquals(L"foo", L"foo"));
}

TEST(StringLowerCaseHash, DifferentWhenLengthDifferent)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_NE(hasher(L"foo"), hasher(L"foobar"));
	EXPECT_NE(hasher(L"FOOBAR"),hasher(L"FOO"));
}

TEST(StringLowerCaseHash, DifferentWhenContentDifferent)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_NE(hasher(L"foo"), hasher(L"bar"));
	EXPECT_NE(hasher(L"FOO"), hasher(L"BAR"));
}

TEST(StringLowerCaseHash, SameWhenCaseDifferent)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_EQ(hasher(L"foo"), hasher(L"FOO"));
	EXPECT_EQ(hasher(L"FOOBAR"), hasher(L"foobar"));
}

TEST(StringLowerCaseHash, SameWhenContentSame)
{
	const Util::impl::string_lowercase_hash hasher;

	EXPECT_EQ(hasher(L"foo"), hasher(L"foo"));
}

TEST(Trim, TrimsLeft)
{
	EXPECT_EQ(Util::Trim(L"\t\v \f\n\rfoo \nbar"), L"foo \nbar");
}

TEST(Trim, TrimsRight)
{
	EXPECT_EQ(Util::Trim(L"foo \nbar\t\r \f\n\v"), L"foo \nbar");
}


TEST(Trim, TrimsLeftRight)
{
	EXPECT_EQ(Util::Trim(L"\t\v \f\n\rfoo \nbar\t\r \f\n\v"), L"foo \nbar");
}

TEST(Trim, TrimsNothing)
{
	EXPECT_EQ(Util::Trim(L"foo \nbar"), L"foo \nbar");
}

TEST(Trim, TrimsAll)
{
	EXPECT_EQ(Util::Trim(L" \f\n\r\t\v"), std::wstring_view{});
}

TEST(TrimInplaceStringView, TrimsLeft)
{
	std::wstring_view str = L"\t\v \f\n\rfoo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceStringView, TrimsRight)
{
	std::wstring_view str = L"foo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceStringView, TrimsLeftRight)
{
	std::wstring_view str = L"\t\v \f\n\rfoo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceStringView, TrimsNothing)
{
	std::wstring_view str = L"foo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceStringView, TrimsAll)
{
	std::wstring_view str = L" \f\n\r\t\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, std::wstring_view{});
}

TEST(TrimInplaceString, TrimsLeft)
{
	std::wstring str = L"\t\v \f\n\rfoo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceString, TrimsRight)
{
	std::wstring str = L"foo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceString, TrimsLeftRight)
{
	std::wstring str = L"\t\v \f\n\rfoo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceString, TrimsNothing)
{
	std::wstring str = L"foo \nbar";
	Util::TrimInplace(str);
	EXPECT_EQ(str, L"foo \nbar");
}

TEST(TrimInplaceString, TrimsAll)
{
	std::wstring str = L" \f\n\r\t\v";
	Util::TrimInplace(str);
	EXPECT_EQ(str, std::wstring{});
}

TEST(StringBeginsWith, ReturnsFalseWhenStringShorterThanPrefix)
{
	EXPECT_FALSE(Util::StringBeginsWith(L"foo", L"foobar"));
}

TEST(StringBeginsWith, ReturnsFalseWhenStringDoesNotBeginsWith)
{
	EXPECT_FALSE(Util::StringBeginsWith(L"foobar", L"bar"));
}

TEST(StringBeginsWith, ReturnsTrueWhenSame)
{
	EXPECT_TRUE(Util::StringBeginsWith(L"foobar", L"foobar"));
}

TEST(StringBeginsWith, ReturnsTrueWhenStringLongerThanPrefixAndBeginsWith)
{
	EXPECT_TRUE(Util::StringBeginsWith(L"foobar", L"foo"));
}

TEST(StringBeginsWithOneOf, ReturnsTrueWhenStringBeginsWithOneOf)
{
	EXPECT_TRUE(Util::StringBeginsWithOneOf(L"foobar", { L"bar", L"foobar", L"foo" }));
}

TEST(StringBeginsWithOneOf, ReturnsFalseWhenStringDoesNotBeginsWithOneOf)
{
	EXPECT_FALSE(Util::StringBeginsWithOneOf(L"buz", { L"bar", L"foobar", L"foo" }));
}

TEST(RemovePrefix, ReturnsPrefixlessStringWhenPrefixPresent)
{
	EXPECT_EQ(Util::RemovePrefix(L"foobar", L"foo"), L"bar");
}

TEST(RemovePrefix, ReturnsSameStringWhenPrefixAbsent)
{
	EXPECT_EQ(Util::RemovePrefix(L"foo", L"bar"), L"foo");
}

TEST(RemovePrefix, ReturnsNullWhenInputEqualsPrefix)
{
	EXPECT_EQ(Util::RemovePrefix(L"foo", L"foo"), std::wstring_view{});
}

TEST(RemovePrefixInplaceStringView, RemovesPrefixWhenPrefixPresent)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"foo");
	EXPECT_EQ(str, L"bar");
}

TEST(RemovePrefixInplaceStringView, DoesNotChangesVariableWhenPrefixAbsent)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"bar");
	EXPECT_EQ(str, L"foobar");
}

TEST(RemovePrefixInplaceStringView, ChangesVariableToNullWhenInputEqualsPrefix)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"foobar");
	EXPECT_EQ(str, std::wstring_view{});
}

TEST(RemovePrefixInplaceString, RemovesPrefixWhenPrefixPresent)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"foo");
	EXPECT_EQ(str, L"bar");
}

TEST(RemovePrefixInplaceString, DoesNotChangesVariableWhenPrefixAbsent)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"bar");
	EXPECT_EQ(str, L"foobar");
}

TEST(RemovePrefixInplaceString, ChangesVariableToNullWhenInputEqualsPrefix)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"foobar");
	EXPECT_EQ(str, std::wstring{});
}