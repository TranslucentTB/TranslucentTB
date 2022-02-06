#include <gtest/gtest.h>

#include "util/hstring_format.hpp"

TEST(Util_hstring_format, FormatsCorrectly)
{
	ASSERT_EQ(Util::hstring_format<L"{:x}">(16), L"10");
}
