#include <gtest/gtest.h>

#include "../../ConfigurationParsers/kvpparser.hpp"

class KeyValueParser_ParseLine : public ::testing::Test {
protected:
	KeyValueParser parser;
};

TEST(KeyValueParser_Constructor, ThrowsWhenDelimiterAndEscapeSame)
{
	EXPECT_THROW(KeyValueParser(L'a', L'a'), std::invalid_argument);
}

TEST_F(KeyValueParser_ParseLine, IgnoresEscapedDelimiterInKey)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello\==world)").first, L"Hello=");
}

TEST_F(KeyValueParser_ParseLine, IgnoresEscapedDelimiterInValue)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello=\=world)").second, L"=world");
}

TEST_F(KeyValueParser_ParseLine, IgnoresEscapedEscapeInKey)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello\\=world)").first, LR"(Hello\)");
}

TEST_F(KeyValueParser_ParseLine, IgnoresEscapedEscapeInValue)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello=\\world)").second, LR"(\world)");
}

TEST_F(KeyValueParser_ParseLine, ThrowsWhenMultipleDelimiter)
{
	EXPECT_THROW(parser.ParseLine(L"Foo=bar=buz"), std::invalid_argument);
}

TEST_F(KeyValueParser_ParseLine, ThrowsWhenNoDelimiter)
{
	EXPECT_THROW(parser.ParseLine(L"Hello world"), std::invalid_argument);
}

TEST_F(KeyValueParser_ParseLine, TrimsKey)
{
	EXPECT_EQ(parser.ParseLine(L" \t Hello \t =world)").first, L"Hello");
}

TEST_F(KeyValueParser_ParseLine, TrimsValue)
{
	EXPECT_EQ(parser.ParseLine(L"Hello= \t world \t ").second, L"world");
}

TEST_F(KeyValueParser_ParseLine, DoesNotIgnoresComment)
{
	EXPECT_EQ(parser.ParseLine(L"Hello=world;test").second, L"world;test");
}