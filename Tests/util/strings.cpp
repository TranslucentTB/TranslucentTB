#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "util/strings.hpp"

using namespace testing;

TEST(Util_IsAscii, ReturnsTrueWhenAscii)
{
	ASSERT_THAT(Util::IsAscii(L'a'), IsTrue());
}

TEST(Util_IsAscii, ReturnsFalseWhenNotAscii)
{
	ASSERT_THAT(Util::IsAscii(L'\u00CB'), IsFalse());
}

TEST(Util_AsciiToUpper, ReturnsUppercaseAsciiFromLowercase)
{
	ASSERT_EQ(Util::AsciiToUpper(L'a'), L'A');
}

TEST(Util_AsciiToUpper, ReturnsUppercaseAsciiFromUppercase)
{
	ASSERT_EQ(Util::AsciiToUpper(L'A'), L'A');
}

TEST(Util_Trim, TrimsLeft)
{
	ASSERT_EQ(Util::Trim(L"\t\v \f\n\rfoo \nbar"), L"foo \nbar");
}

TEST(Util_Trim, TrimsRight)
{
	ASSERT_EQ(Util::Trim(L"foo \nbar\t\r \f\n\v"), L"foo \nbar");
}


TEST(Util_Trim, TrimsLeftRight)
{
	ASSERT_EQ(Util::Trim(L"\t\v \f\n\rfoo \nbar\t\r \f\n\v"), L"foo \nbar");
}

TEST(Util_Trim, TrimsNothing)
{
	ASSERT_EQ(Util::Trim(L"foo \nbar"), L"foo \nbar");
}

TEST(Util_Trim, TrimsAll)
{
	ASSERT_THAT(Util::Trim(L" \f\n\r\t\v"), IsEmpty());
}

TEST(Util_TrimInplace_StringView, TrimsLeft)
{
	std::wstring_view str = L"\t\v \f\n\rfoo \nbar";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsRight)
{
	std::wstring_view str = L"foo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsLeftRight)
{
	std::wstring_view str = L"\t\v \f\n\rfoo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsNothing)
{
	std::wstring_view str = L"foo \nbar";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_StringView, TrimsAll)
{
	std::wstring_view str = L" \f\n\r\t\v";
	Util::TrimInplace(str);
	ASSERT_THAT(str, IsEmpty());
}

TEST(Util_TrimInplace_String, TrimsLeft)
{
	std::wstring str = L"\t\v \f\n\rfoo \nbar";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsRight)
{
	std::wstring str = L"foo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsLeftRight)
{
	std::wstring str = L"\t\v \f\n\rfoo \nbar\t\r \f\n\v";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsNothing)
{
	std::wstring str = L"foo \nbar";
	Util::TrimInplace(str);
	ASSERT_EQ(str, L"foo \nbar");
}

TEST(Util_TrimInplace_String, TrimsAll)
{
	std::wstring str = L" \f\n\r\t\v";
	Util::TrimInplace(str);
	ASSERT_THAT(str, IsEmpty());
}

TEST(Util_StringBeginsWithOneOf, ReturnsTrueWhenStringBeginsWithOneOf)
{
	ASSERT_THAT(Util::StringBeginsWithOneOf(L"foobar", { L"bar", L"foobar", L"foo" }), IsTrue());
}

TEST(Util_StringBeginsWithOneOf, ReturnsFalseWhenStringDoesNotBeginsWithOneOf)
{
	ASSERT_THAT(Util::StringBeginsWithOneOf(L"buz", { L"bar", L"foobar", L"foo" }), IsFalse());
}

TEST(Util_RemovePrefix, ReturnsPrefixlessStringWhenPrefixPresent)
{
	ASSERT_EQ(Util::RemovePrefix(L"foobar", L"foo"), L"bar");
}

TEST(Util_RemovePrefix, ReturnsSameStringWhenPrefixAbsent)
{
	ASSERT_EQ(Util::RemovePrefix(L"foo", L"bar"), L"foo");
}

TEST(Util_RemovePrefix, ReturnsNullWhenInputEqualsPrefix)
{
	ASSERT_THAT(Util::RemovePrefix(L"foo", L"foo"), IsEmpty());
}

TEST(Util_RemovePrefixInplace_StringView, RemovesPrefixWhenPrefixPresent)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"foo");
	ASSERT_EQ(str, L"bar");
}

TEST(Util_RemovePrefixInplace_StringView, DoesNotChangesVariableWhenPrefixAbsent)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"bar");
	ASSERT_EQ(str, L"foobar");
}

TEST(Util_RemovePrefixInplace_StringView, ChangesVariableToNullWhenInputEqualsPrefix)
{
	std::wstring_view str = L"foobar";
	Util::RemovePrefixInplace(str, L"foobar");
	ASSERT_THAT(str, IsEmpty());
}

TEST(Util_RemovePrefixInplace_String, RemovesPrefixWhenPrefixPresent)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"foo");
	ASSERT_EQ(str, L"bar");
}

TEST(Util_RemovePrefixInplace_String, DoesNotChangesVariableWhenPrefixAbsent)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"bar");
	ASSERT_EQ(str, L"foobar");
}

TEST(Util_RemovePrefixInplace_String, ChangesVariableToNullWhenInputEqualsPrefix)
{
	std::wstring str = L"foobar";
	Util::RemovePrefixInplace(str, L"foobar");
	ASSERT_THAT(str, IsEmpty());
}
