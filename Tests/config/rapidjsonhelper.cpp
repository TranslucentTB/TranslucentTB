#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <ranges>

#include "config/rapidjsonhelper.hpp"

namespace {
	static constexpr rj::Type types[] = {
		rj::Type::kNullType,
		rj::Type::kObjectType,
		rj::Type::kArrayType,
		rj::Type::kStringType,
		rj::Type::kNumberType
	};

	static constexpr rj::Type boolTypes[] = {
		rj::Type::kFalseType,
		rj::Type::kTrueType
	};

	static constexpr std::wstring_view testObj = L"test object";
	static constexpr std::wstring_view testKey = L"test_key";

	enum class TestEnum : std::uint32_t {
		Foo = 0,
		Bar = 1,
		Buz = 2,
		Quux = 3,
		Junk = static_cast<std::uint32_t>(-1)
	};

	static constexpr std::array<std::wstring_view, 4> testEnumNameMapping = {
		L"foo",
		L"bar",
		L"buz",
		L"quux"
	};

	constexpr std::wstring_view TestEnumName(TestEnum value)
	{
		return testEnumNameMapping.at(static_cast<std::size_t>(value));
	}

	struct WriterMock {
		MOCK_METHOD(bool, Key, (const wchar_t *str, rj::SizeType length));
		MOCK_METHOD(bool, String, (const wchar_t *str, rj::SizeType length));
		MOCK_METHOD(bool, Bool, (bool value));
		MOCK_METHOD(bool, StartObject, ());
		MOCK_METHOD(bool, EndObject, ());
	};

	struct JsonObjectMock {
		MOCK_METHOD(void, Serialize, (WriterMock &writer), (const));
		MOCK_METHOD(void, Deserialize, (const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view)));
	};

	void UnknownKeyCallback(std::wstring_view) noexcept
	{
	}

	class SameString {
		std::wstring_view m_MatchStr;

		void DescribeHelper(bool matches, std::ostream *os) const
		{
			*os << "is";
			if (!matches)
			{
				*os << "n't";
			}

			*os << " equal to " << testing::PrintToString(std::wstring(m_MatchStr));
		}

	public:
		using is_gtest_matcher = void;

		constexpr SameString(std::wstring_view str) noexcept : m_MatchStr(str) { }

		constexpr bool MatchAndExplain(const std::tuple<const wchar_t *, rj::SizeType> &args, std::ostream *) const noexcept
		{
			return m_MatchStr == std::wstring_view { std::get<0>(args), std::get<1>(args) };
		}

		void DescribeTo(std::ostream *os) const { DescribeHelper(true, os); }
		void DescribeNegationTo(std::ostream *os) const { DescribeHelper(false, os); }
	};
}

TEST(RapidJSONHelper_TypeChecks, ReturnsTrueOnSameType)
{
	for (const auto type : types)
	{
		ASSERT_TRUE(rjh::IsType(type, type));
		ASSERT_NO_THROW(rjh::EnsureType(type, type, testObj));
	}

	for (const auto type : boolTypes)
	{
		ASSERT_TRUE(rjh::IsType(type, type));
		ASSERT_NO_THROW(rjh::EnsureType(type, type, testObj));
	}
}

TEST(RapidJSONHelper_TypeChecks, ReturnsTrueOnMismatchingBoolTypes)
{
	ASSERT_TRUE(rjh::IsType(rj::Type::kFalseType, rj::Type::kTrueType));
	ASSERT_TRUE(rjh::IsType(rj::Type::kTrueType, rj::Type::kFalseType));

	ASSERT_NO_THROW(rjh::EnsureType(rj::Type::kFalseType, rj::Type::kTrueType, testObj));
	ASSERT_NO_THROW(rjh::EnsureType(rj::Type::kTrueType, rj::Type::kFalseType, testObj));
}

TEST(RapidJSONHelper_TypeChecks, ReturnsFalseOnMismatchingTypes)
{
	for (std::size_t i = 0; i < std::size(types); ++i)
	{
		for (std::size_t j = i + 1; j < std::size(types); ++j)
		{
			ASSERT_FALSE(rjh::IsType(types[i], types[j]));
			ASSERT_THROW(rjh::EnsureType(types[i], types[j], testObj), rjh::DeserializationError);
		}
	}

	for (const auto type : types)
	{
		for (const auto boolType : boolTypes)
		{
			ASSERT_FALSE(rjh::IsType(type, boolType));
			ASSERT_THROW(rjh::EnsureType(type, boolType, testObj), rjh::DeserializationError);
		}
	}
}

TEST(RapidJSONHelper_Strings, ConvertsValueToStringView)
{
	const rjh::value_t testValue(L"foo");

	ASSERT_EQ(rjh::ValueToStringView(testValue), L"foo");
}

TEST(RapidJSONHelper_Strings, ConvertsStringViewToValue)
{
	const std::wstring_view testStrView(L"foo");
	const auto resultValue = rjh::StringViewToValue(testStrView);

	ASSERT_EQ(testStrView, resultValue.Get<std::wstring>());
}

TEST(RapidJSONHelper_Serialize, WriteKeyWritesKey)
{
	WriterMock mock;
	EXPECT_CALL(mock, Key).With(SameString(testKey));

	rjh::WriteKey(mock, testKey);
}

TEST(RapidJSONHelper_Serialize, WriteStringWritesString)
{
	WriterMock mock;
	EXPECT_CALL(mock, String).With(SameString(testObj));

	rjh::WriteString(mock, testObj);
}

TEST(RapidJSONHelper_Serialize, WritesBool)
{
	for (const bool value : { true, false })
	{
		testing::InSequence s;

		WriterMock mock;
		EXPECT_CALL(mock, Key).With(SameString(testKey));
		EXPECT_CALL(mock, Bool(value));

		rjh::Serialize(mock, value, testKey);
	}
}

TEST(RapidJSONHelper_Serialize, WritesString)
{
	testing::InSequence s;

	WriterMock mock;
	EXPECT_CALL(mock, Key).With(SameString(testKey));
	EXPECT_CALL(mock, String).With(SameString(testObj));

	rjh::Serialize(mock, testObj, testKey);
}

TEST(RapidJSONHelper_Serialize, WritesEnum)
{
	for (const TestEnum value : { TestEnum::Foo, TestEnum::Bar, TestEnum::Buz, TestEnum::Quux })
	{
		testing::InSequence s;

		WriterMock mock;
		EXPECT_CALL(mock, Key).With(SameString(testKey));
		EXPECT_CALL(mock, String).With(SameString(TestEnumName(value)));

		rjh::Serialize(mock, value, testKey, testEnumNameMapping);
	}
}

TEST(RapidJSONHelper_Serialize, IgnoresUnknownEnumValues)
{
	WriterMock mock;
	EXPECT_CALL(mock, Key).Times(0);
	EXPECT_CALL(mock, String).Times(0);

	rjh::Serialize(mock, TestEnum::Junk, testKey, testEnumNameMapping);
}

TEST(RapidJSONHelper_Serialize, WritesObject)
{
	testing::InSequence s;

	WriterMock writerMock;
	JsonObjectMock objectMock;

	EXPECT_CALL(writerMock, Key).With(SameString(testKey));
	EXPECT_CALL(writerMock, StartObject);
	EXPECT_CALL(objectMock, Serialize(testing::Ref(writerMock)));
	EXPECT_CALL(writerMock, EndObject);

	rjh::Serialize(writerMock, objectMock, testKey);
}

TEST(RapidJSONHelper_Serialize, WritesOptional)
{
	std::optional<std::wstring> opt(testObj);

	WriterMock mock;
	EXPECT_CALL(mock, Key).With(SameString(testKey));
	EXPECT_CALL(mock, String).With(SameString(testObj));

	rjh::Serialize(mock, opt, testKey);
}

TEST(RapidJSONHelper_Serialize, IgnoresEmptyOptional)
{
	std::optional<std::wstring> opt;

	WriterMock mock;
	EXPECT_CALL(mock, Key).Times(0);
	EXPECT_CALL(mock, String).Times(0);

	rjh::Serialize(mock, opt, testKey);
}

TEST(RapidJSONHelper_Deserialize, DeserializesBool)
{
	for (const bool expected : { true, false })
	{
		const rjh::value_t value(expected);
		bool member = !expected;
		rjh::Deserialize(value, member, testObj);
		ASSERT_EQ(member, expected);
	}
}

TEST(RapidJSONHelper_Deserialize, DeserializesEnum)
{
	for (const auto expected : { TestEnum::Foo, TestEnum::Bar, TestEnum::Buz, TestEnum::Quux })
	{
		const std::wstring_view str = TestEnumName(expected);
		const rjh::value_t value(rjh::StringViewToValue(str));

		TestEnum test = TestEnum::Junk;
		rjh::Deserialize(value, test, testObj, testEnumNameMapping);
		ASSERT_EQ(test, expected);
	}
}

TEST(RapidJSONHelper_Deserialize, ThrowsOnInvalidEnumString)
{
	const rjh::value_t value(L"invalid");
	TestEnum test;
	try
	{
		rjh::Deserialize(value, test, testObj, testEnumNameMapping);
		FAIL();
	}
	catch (const rjh::DeserializationError &err)
	{
		ASSERT_EQ(err.what, L"Found invalid enum string \"invalid\" while deserializing key \"test object\"");
	}
}

TEST(RapidJSONHelper_Deserialize, DeserializesClass)
{
	const rjh::value_t value(rj::Type::kObjectType);

	JsonObjectMock mock;
	EXPECT_CALL(mock, Deserialize(testing::Ref(value), UnknownKeyCallback));
	
	rjh::Deserialize(value, mock, testObj, UnknownKeyCallback);
}

TEST(RapidJSONHelper_Deserialize, DeserializesOptional)
{
	const rjh::value_t value(true);
	std::optional<bool> opt;
	rjh::Deserialize(value, opt, testObj);

	ASSERT_TRUE(opt.value_or(false));
}
