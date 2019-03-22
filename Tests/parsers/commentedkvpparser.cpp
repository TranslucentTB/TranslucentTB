#include <gtest/gtest.h>

#include "../../ConfigurationParsers/commentedkvpparser.hpp"

class CommentedKeyValueParser_ParseLine : public ::testing::Test {
protected:
	CommentedKeyValueParser parser;
};

TEST(CommentedKeyValueParser_Constructor, ThrowsWhenDelimiterAndEscapeSame)
{
	EXPECT_THROW(CommentedKeyValueParser(L'a', L'a'), std::invalid_argument);
}

TEST(CommentedKeyValueParser_Constructor, ThrowsWhenDelimiterAndCommentSame)
{
	EXPECT_THROW(CommentedKeyValueParser(L'a', L'\\', L'a'), std::invalid_argument);
}

TEST(CommentedKeyValueParser_Constructor, ThrowsWhenEscapeAndCommentSame)
{
	EXPECT_THROW(CommentedKeyValueParser(L'=', L'a', L'a'), std::invalid_argument);
}

TEST_F(CommentedKeyValueParser_ParseLine, IgnoresEscapedDelimiterInKey)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello\==world)").first, L"Hello=");
}

TEST_F(CommentedKeyValueParser_ParseLine, IgnoresEscapedDelimiterInValue)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello=\=world)").second, L"=world");
}

TEST_F(CommentedKeyValueParser_ParseLine, IgnoresEscapedEscapeInKey)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello\\=world)").first, LR"(Hello\)");
}

TEST_F(CommentedKeyValueParser_ParseLine, IgnoresEscapedEscapeInValue)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello=\\world)").second, LR"(\world)");
}

TEST_F(CommentedKeyValueParser_ParseLine, IgnoresEscapedCommentInKey)
{
	EXPECT_EQ(parser.ParseLine(LR"(\;Hello=world)").first, L";Hello");
}

TEST_F(CommentedKeyValueParser_ParseLine, IgnoresEscapedCommentInValue)
{
	EXPECT_EQ(parser.ParseLine(LR"(Hello=\;world)").second, L";world");
}

TEST_F(CommentedKeyValueParser_ParseLine, ThrowsWhenMultipleDelimiter)
{
	EXPECT_THROW(parser.ParseLine(L"Foo=bar=buz"), std::invalid_argument);
}

TEST_F(CommentedKeyValueParser_ParseLine, ThrowsWhenNoDelimiter)
{
	EXPECT_THROW(parser.ParseLine(L"Hello world"), std::invalid_argument);
}

TEST_F(CommentedKeyValueParser_ParseLine, TrimsKey)
{
	EXPECT_EQ(parser.ParseLine(L" \t Hello \t =world)").first, L"Hello");
}

TEST_F(CommentedKeyValueParser_ParseLine, TrimsValue)
{
	EXPECT_EQ(parser.ParseLine(L"Hello= \t world \t ").second, L"world");
}

TEST_F(CommentedKeyValueParser_ParseLine, IgnoresComment)
{
	EXPECT_EQ(parser.ParseLine(L"Hello=world;test").second, L"world");
}