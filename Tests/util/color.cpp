#include <gtest/gtest.h>
#include <ranges>

#include "util/color.hpp"

TEST(Util_HsvColor_Constructor, DefaultConstructorIsTransparentBlack)
{
	const Util::HsvColor col;
	ASSERT_EQ(col.H, 0.0);
	ASSERT_EQ(col.S, 0.0);
	ASSERT_EQ(col.V, 0.0);
	ASSERT_EQ(col.A, 0.0);
}

TEST(Util_HsvColor_Constructor, ConstructorFromHSVUsesFullAlpha)
{
	const Util::HsvColor col = { 67.0, 0.68, 0.69 };
	ASSERT_EQ(col.H, 67.0);
	ASSERT_EQ(col.S, 0.68);
	ASSERT_EQ(col.V, 0.69);
	ASSERT_EQ(col.A, 1.0);
}

TEST(Util_HsvColor_Constructor, ConstructorFromHSVAGivesCorrectValue)
{
	const Util::HsvColor col = { 67.0, 0.68, 0.69, 0.70 };
	ASSERT_EQ(col.H, 67.0);
	ASSERT_EQ(col.S, 0.68);
	ASSERT_EQ(col.V, 0.69);
	ASSERT_EQ(col.A, 0.70);
}

TEST(Util_HsvColor_Constructor, ConstructorFromWinRTGivesCorrectValue)
{
	const Util::HsvColor col = txmp::HsvColor { 67.0, 0.68, 0.69, 0.70 };
	ASSERT_EQ(col.H, 67.0);
	ASSERT_EQ(col.S, 0.68);
	ASSERT_EQ(col.V, 0.69);
	ASSERT_EQ(col.A, 0.70);
}

TEST(Util_HsvColor_Constructor, ConstructorFromFloat4GivesCorrectValue)
{
	const Util::HsvColor col = wf::Numerics::float4 { 67.0f, 0.68f, 0.69f, 0.70f };
	ASSERT_EQ(col.H, 67.0);
	ASSERT_EQ(col.S, 0.68f);
	ASSERT_EQ(col.V, 0.69f);
	ASSERT_EQ(col.A, 0.70f);
}

TEST(Util_HsvColor_ToWinRT, ConvertsToSameColor)
{
	const txmp::HsvColor convertedCol = Util::HsvColor { 67.0, 0.68, 0.69, 0.70 };
	const txmp::HsvColor originalCol = { .H = 67.0, .S = 0.68, .V = 0.69, .A = 0.70 };

	ASSERT_EQ(convertedCol.H, originalCol.H);
	ASSERT_EQ(convertedCol.S, originalCol.S);
	ASSERT_EQ(convertedCol.V, originalCol.V);
	ASSERT_EQ(convertedCol.A, originalCol.A);
}

TEST(Util_HsvColor_ToFloat4, ConvertsToSameColor)
{
	const wf::Numerics::float4 convertedCol = Util::HsvColor { 67.0, 0.68, 0.69, 0.70 };
	const wf::Numerics::float4 originalCol = { 67.0f, 0.68f, 0.69f, 0.70f };

	ASSERT_EQ(convertedCol.x, originalCol.x);
	ASSERT_EQ(convertedCol.y, originalCol.y);
	ASSERT_EQ(convertedCol.z, originalCol.z);
	ASSERT_EQ(convertedCol.w, originalCol.w);
}

TEST(Util_Color_Constructor, DefaultConstructorIsTransparentBlack)
{
	ASSERT_EQ(Util::Color(), Util::Color(0x00, 0x00, 0x00, 0x00));
}

TEST(Util_Color_Constructor, ConstructorFromRGBUsesFullAlpha)
{
	ASSERT_EQ(Util::Color(0x00, 0x00, 0x00), Util::Color(0x00, 0x00, 0x00, 0xFF));
}

TEST(Util_Color_Constructor, ConstructorFromRGBAGivesCorrectValue)
{
	const Util::Color col = { 0xDE, 0xAD, 0xBE, 0xEF };
	ASSERT_EQ(col.R, 0xDE);
	ASSERT_EQ(col.G, 0xAD);
	ASSERT_EQ(col.B, 0xBE);
	ASSERT_EQ(col.A, 0xEF);
}

TEST(Util_Color_Constructor, ConstructorFromWinRTGivesCorrectValue)
{
	const winrt::Windows::UI::Color winrtCol = { .A = 0xEF, .R = 0xDE, .G = 0xAD, .B = 0xBE };
	ASSERT_EQ(Util::Color(winrtCol), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_ToRGBA, ReturnsCorrectValue)
{
	ASSERT_EQ(Util::Color(0xDE, 0xAD, 0xBE, 0xEF).ToRGBA(), 0xDEADBEEF);
}

TEST(Util_Color_ToABGR, ReturnsCorrectValue)
{
	ASSERT_EQ(Util::Color(0xDE, 0xAD, 0xBE, 0xEF).ToABGR(), 0xEFBEADDE);
}

TEST(Util_Color_Premultiply, ReturnsCorrectValue)
{
	// [0, 255)
	for (int i : std::views::iota(0, 256))
	{
		for (int j : std::views::iota(0, 256))
		{
			const auto color = static_cast<uint8_t>(i);
			const auto alpha = static_cast<uint8_t>(j);

			const auto expected = static_cast<uint8_t>(color * alpha / 255);
			ASSERT_EQ(Util::Color(color, 0, 255, alpha).Premultiply(), Util::Color(expected, 0, alpha, alpha));
		}
	}
}

TEST(Util_Color_ToHSV, ReturnsCorrectValue)
{
	static constexpr std::pair<Util::Color, Util::HsvColor> cases[] = {
		{ { 170, 204, 153, 255 }, { 100.0, 0.25, 0.80, 1.0 } },
		{ { 255, 105, 180, 255 }, { 330.0, 0.58823529411764708, 1.0, 1.0 } }
	};

	for (const auto &testCase : cases)
	{
		const auto result = testCase.first.ToHSV();
		ASSERT_DOUBLE_EQ(result.H, testCase.second.H);
		ASSERT_DOUBLE_EQ(result.S, testCase.second.S);
		ASSERT_DOUBLE_EQ(result.V, testCase.second.V);
		ASSERT_DOUBLE_EQ(result.A, testCase.second.A);
	}
}

TEST(Util_Color_ToString, ReturnsCorrectString)
{
	static constexpr std::pair<Util::Color, std::wstring_view> cases[] = {
		{ { 0xDE, 0xAD, 0xBE, 0xEF }, L"#DEADBEEF" },
		{ { 0x00, 0x00, 0xFF, 0x00 }, L"#0000FF00" },
		{ { 0x00, 0x00, 0x00, 0x00 }, L"#00000000" },
		{ { 0xFF, 0xFF, 0xFF, 0xFF }, L"#FFFFFFFF" },
	};

	for (const auto &testCase : cases)
	{
		ASSERT_EQ(testCase.first.ToString(), testCase.second);
	}
}

TEST(Util_Color_FromString, ParsesColor)
{
	static constexpr std::tuple<std::wstring_view, bool, Util::Color> cases[] = {
		{ L"#FAF", false, { 0xFF, 0xAA, 0xFF } },
		{ L"#DEAD", false, { 0xDD, 0xEE, 0xAA, 0xDD } },
		{ L"#C0FFEE", false, { 0xC0, 0xFF, 0xEE } },
		{ L"#DEADBEEF", false, { 0xDE, 0xAD, 0xBE, 0xEF } },
		{ L"   #FFFFFF \t \n", false, { 0xFF, 0xFF, 0xFF } },
		{ L"#FAF", true, { 0xFF, 0xAA, 0xFF } },
		{ L"#DEAD", true, { 0xDD, 0xEE, 0xAA, 0xDD } },
		{ L"#C0FFEE", true, { 0xC0, 0xFF, 0xEE } },
		{ L"#DEADBEEF", true, { 0xDE, 0xAD, 0xBE, 0xEF } },
		{ L"   #FFFFFF \t \n", true, { 0xFF, 0xFF, 0xFF } },
		{ L"FAF", true, { 0xFF, 0xAA, 0xFF } },
		{ L"DEAD", true, { 0xDD, 0xEE, 0xAA, 0xDD } },
		{ L"C0FFEE", true, { 0xC0, 0xFF, 0xEE } },
		{ L"DEADBEEF", true, { 0xDE, 0xAD, 0xBE, 0xEF } },
		{ L"   FFFFFF \t \n", true, { 0xFF, 0xFF, 0xFF } }
	};

	for (const auto [str, allowNoPrefix, expected] : cases)
	{
		ASSERT_EQ(Util::Color::FromString(str, allowNoPrefix), expected);
	}
}

TEST(Util_Color_FromString, ThrowsWhenInvalidColor)
{
	static constexpr std::wstring_view cases[] = {
		L"FFFFFF",
		L"#FFFFFFF",
		L"#",
		L"",
		L"  \n \t \r #   \n"
	};

	for (const auto &testCase : cases)
	{
		ASSERT_THROW(Util::Color::FromString(testCase), std::invalid_argument);
	}
}

TEST(Util_Color_FromString, AllowNoPrefixParameter)
{
	ASSERT_EQ(Util::Color::FromString(L"FFFFFF", true), Util::Color(0xFF, 0xFF, 0xFF));
}

TEST(Util_Color_FromRGBA, ReturnsCorrectValue)
{
	ASSERT_EQ(Util::Color::FromRGBA(0xDEADBEEF), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_FromABGR, ReturnsCorrectValue)
{
	ASSERT_EQ(Util::Color::FromABGR(0xEFBEADDE), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_FromHSV, ReturnsCorrectValue)
{
	static constexpr std::pair<std::tuple<double, double, double>, Util::Color> cases[] = {
		{ { 0.0, 1.0, 1.0 }, { 0xFF, 0x00, 0x00 } },
		{ { 0.0, 0.0, 1.0 }, { 0xFF, 0xFF, 0xFF } }
	};

	for (const auto &testCase : cases)
	{
		const auto [h, s, v] = testCase.first;
		ASSERT_EQ(Util::Color::FromHSV(h, s, v), testCase.second);
	}
}

TEST(Util_Color_FromHSV, ThrowsWhenInvalidHue)
{
	static constexpr double cases[] = {
		-0.1,
		360.1,
		1337
	};

	for (const auto &testCase : cases)
	{
		ASSERT_THROW(Util::Color::FromHSV(testCase, 0.0, 0.0), std::out_of_range);
	}
}

TEST(Util_Color_ToWinRT, ConvertsToSameColor)
{
	const winrt::Windows::UI::Color convertedCol = Util::Color { 0xDE, 0xAD, 0xBE, 0xEF };
	const winrt::Windows::UI::Color originalCol = { .A = 0xEF, .R = 0xDE, .G = 0xAD, .B = 0xBE };

	ASSERT_EQ(convertedCol, originalCol);
}

TEST(Util_Color_Equality, ReturnsTrueWhenSame)
{
	ASSERT_EQ(Util::Color(0xDE, 0xAD, 0xBE, 0xEF), Util::Color(0xDE, 0xAD, 0xBE, 0xEF));
}

TEST(Util_Color_Equality, ReturnsFalseWhenDifferent)
{
	ASSERT_NE(Util::Color(0xDE, 0xAD, 0xBE, 0xEF), Util::Color(0xC0, 0xFF, 0xEE, 0x00));
}
