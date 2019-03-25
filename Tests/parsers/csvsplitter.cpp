#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../../ConfigurationParsers/csvsplitter.hpp"

using namespace testing;

class CharacterSeparatedValuesSplitter_SplitLine : public Test {
protected:
	CharacterSeparatedValuesSplitter splitter;
};

TEST(CharacterSeparatedValuesSplitter_Constructor, ThrowsWhenSeparatorAndEscapeSame)
{
	ASSERT_THROW(CharacterSeparatedValuesSplitter(L'a', L'a'), std::invalid_argument);
}

TEST(CharacterSeparatedValuesSplitter_Constructor, ThrowsWhenSeparatorAndCommentSame)
{
	ASSERT_THROW(CharacterSeparatedValuesSplitter(L'a', L'\\', L'a'), std::invalid_argument);
}

TEST(CharacterSeparatedValuesSplitter_Constructor, ThrowsWhenEscapeAndCommentSame)
{
	ASSERT_THROW(CharacterSeparatedValuesSplitter(L'=', L'a', L'a'), std::invalid_argument);
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, IncludesEscapedSeparator)
{
	ASSERT_THAT(splitter.SplitLine(LR"(test\,test,test)"), ElementsAre(L"test,test", L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, IncludesEscapedEscape)
{
	ASSERT_THAT(splitter.SplitLine(LR"(test\\test,test)"), ElementsAre(LR"(test\test)", L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, IncludesEscapedComment)
{
	ASSERT_THAT(splitter.SplitLine(LR"(test\;test,test)"), ElementsAre(L"test;test", L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, TrimsValues)
{
	ASSERT_THAT(splitter.SplitLine(L"  \t test , test , test \t  "), ElementsAre(L"test", L"test", L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, IgnoresComment)
{
	ASSERT_THAT(splitter.SplitLine(L"test;test,test"), ElementsAre(L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, ReturnsEmptyVectorOnEmptyInput)
{
	ASSERT_THAT(splitter.SplitLine({ }), IsEmpty());
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, ReturnsEmptyVectorIfAllInputTrimmed)
{
	ASSERT_THAT(splitter.SplitLine(L"   \t  "), IsEmpty());
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, ReturnsEmptyVectorOnWholeLineComment)
{
	ASSERT_THAT(splitter.SplitLine(L";test"), IsEmpty());
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, ReturnsSingleElementVectorWhenNoSeparator)
{
	ASSERT_THAT(splitter.SplitLine(L"test"), ElementsAre(L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, ReturnsEmptyStringWhenTwoConsecutiveSeparators)
{
	ASSERT_THAT(splitter.SplitLine(L"test,,test"), ElementsAre(L"test", L"", L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, FirstElementIsEmptyStringWhenFirstCharacterIsSeparator)
{
	ASSERT_THAT(splitter.SplitLine(L",test,test"), ElementsAre(L"", L"test", L"test"));
}

TEST_F(CharacterSeparatedValuesSplitter_SplitLine, LastElementIsEmptyStringWhenLastCharacterIsSeparator)
{
	ASSERT_THAT(splitter.SplitLine(L"test,test,"), ElementsAre(L"test", L"test", L""));
}