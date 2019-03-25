#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../../ConfigurationParsers/kvpparser.hpp"

using namespace testing;

class KeyValueParser_ParseLine : public Test {
protected:
	KeyValueParser parser;
};

TEST(KeyValueParser_Constructor, ThrowsWhenDelimiterAndEscapeSame)
{
	ASSERT_THROW(KeyValueParser(L'a', L'a'), std::invalid_argument);
}

TEST(KeyValueParser_Constructor, ThrowsWhenDelimiterAndCommentSame)
{
	ASSERT_THROW(KeyValueParser(L'a', L'\\', L'a'), std::invalid_argument);
}

TEST(KeyValueParser_Constructor, ThrowsWhenEscapeAndCommentSame)
{
	ASSERT_THROW(KeyValueParser(L'=', L'a', L'a'), std::invalid_argument);
}

TEST_F(KeyValueParser_ParseLine, IncludesEscapedDelimiterInKey)
{
	ASSERT_EQ(parser.ParseLine(LR"(Hello\==world)").first, L"Hello=");
}

TEST_F(KeyValueParser_ParseLine, IncludesEscapedDelimiterInValue)
{
	ASSERT_EQ(parser.ParseLine(LR"(Hello=\=world)").second, L"=world");
}

TEST_F(KeyValueParser_ParseLine, IncludesEscapedEscapeInKey)
{
	ASSERT_EQ(parser.ParseLine(LR"(Hello\\=world)").first, LR"(Hello\)");
}

TEST_F(KeyValueParser_ParseLine, IncludesEscapedEscapeInValue)
{
	ASSERT_EQ(parser.ParseLine(LR"(Hello=\\world)").second, LR"(\world)");
}

TEST_F(KeyValueParser_ParseLine, IncludesEscapedCommentInKey)
{
	ASSERT_EQ(parser.ParseLine(LR"(\;Hello=world)").first, L";Hello");
}

TEST_F(KeyValueParser_ParseLine, IncludesEscapedCommentInValue)
{
	ASSERT_EQ(parser.ParseLine(LR"(Hello=\;world)").second, L";world");
}

TEST_F(KeyValueParser_ParseLine, ThrowsWhenMultipleDelimiters)
{
	ASSERT_THROW(parser.ParseLine(L"Foo=bar=buz"), std::invalid_argument);
}

TEST_F(KeyValueParser_ParseLine, ThrowsWhenNoDelimiter)
{
	ASSERT_THROW(parser.ParseLine(L"Hello world"), std::invalid_argument);
}

TEST_F(KeyValueParser_ParseLine, TrimsKey)
{
	ASSERT_EQ(parser.ParseLine(L" \t Hello \t =world)").first, L"Hello");
}

TEST_F(KeyValueParser_ParseLine, TrimsValue)
{
	ASSERT_EQ(parser.ParseLine(L"Hello= \t world \t ").second, L"world");
}

TEST_F(KeyValueParser_ParseLine, IgnoresComment)
{
	ASSERT_EQ(parser.ParseLine(L"Hello=world;test").second, L"world");
}

TEST_F(KeyValueParser_ParseLine, KeyIsEmptyOnEmptyLine)
{
	ASSERT_THAT(parser.ParseLine({ }).first, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, ValueIsEmptyOnEmptyLine)
{
	ASSERT_THAT(parser.ParseLine({ }).second, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, KeyIsEmptyIfAllInputTrimmed)
{
	ASSERT_THAT(parser.ParseLine(L"    \t   ").first, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, ValueIsEmptyIfAllInputTrimmed)
{
	ASSERT_THAT(parser.ParseLine(L"    \t   ").second, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, KeyIsEmptyOnWholeLineComment)
{
	ASSERT_THAT(parser.ParseLine(L";test").first, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, ValueIsEmptyOnWholeLineComment)
{
	ASSERT_THAT(parser.ParseLine(L";test").second, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, KeyIsEmptyOnNoTextBeforeDelimiter)
{
	ASSERT_THAT(parser.ParseLine(L"=world").first, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, ValueIsEmptyOnNoTextAfterDelimiter)
{
	ASSERT_THAT(parser.ParseLine(L"Hello=").second, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, KeyIsEmptyOnNoTextExceptDelimiter)
{
	ASSERT_THAT(parser.ParseLine(L"=").first, IsEmpty());
}

TEST_F(KeyValueParser_ParseLine, ValueIsEmptyOnNoTextExceptDelimiter)
{
	ASSERT_THAT(parser.ParseLine(L"=").second, IsEmpty());
}