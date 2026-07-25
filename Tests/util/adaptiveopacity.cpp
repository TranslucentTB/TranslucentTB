#include <array>
#include <gtest/gtest.h>

#include "util/adaptiveopacity.hpp"

using namespace Util::AdaptiveOpacity;

TEST(Util_AdaptiveOpacity, RelativeLuminanceUsesLinearSrgb)
{
	EXPECT_NEAR(RelativeLuminance({ 255, 255, 255 }), 1.0, 1e-6);
	EXPECT_NEAR(RelativeLuminance({ 0, 0, 0 }), 0.0, 1e-6);
	EXPECT_NEAR(RelativeLuminance({ 127, 127, 127 }), 0.2122, 0.002);
}

TEST(Util_AdaptiveOpacity, EmptySamplesHaveNoBrightnessPressure)
{
	EXPECT_DOUBLE_EQ(BrightnessPressure({}), 0.0);
}

TEST(Util_AdaptiveOpacity, DarkSamplesKeepConfiguredAlpha)
{
	const std::array samples {
		Util::Color { 0, 0, 0 },
		Util::Color { 32, 32, 32 },
		Util::Color { 64, 64, 64 }
	};

	EXPECT_EQ(TargetAlpha(96, samples), 96);
}

TEST(Util_AdaptiveOpacity, WhiteSamplesReachFullAlpha)
{
	const std::array samples {
		Util::Color { 255, 255, 255 },
		Util::Color { 250, 250, 250 }
	};

	EXPECT_EQ(TargetAlpha(96, samples), 255);
}

TEST(Util_AdaptiveOpacity, WhiteCoverageProtectsPartOfTaskbar)
{
	const std::array samples {
		Util::Color { 20, 20, 20 },
		Util::Color { 20, 20, 20 },
		Util::Color { 20, 20, 20 },
		Util::Color { 255, 255, 255 }
	};

	EXPECT_EQ(TargetAlpha(96, samples), 255);
}

TEST(Util_AdaptiveOpacity, BrightnessResponseIsMonotonic)
{
	const std::array gray { Util::Color { 160, 160, 160 } };
	const std::array bright { Util::Color { 210, 210, 210 } };
	const std::array white { Util::Color { 255, 255, 255 } };

	EXPECT_LE(TargetAlpha(96, gray), TargetAlpha(96, bright));
	EXPECT_LE(TargetAlpha(96, bright), TargetAlpha(96, white));
}

TEST(Util_AdaptiveOpacity, AlphaNeverDropsBelowConfiguredFloor)
{
	const std::array dark { Util::Color { 0, 0, 0 } };
	const std::array white { Util::Color { 255, 255, 255 } };

	EXPECT_EQ(TargetAlpha(220, dark), 220);
	EXPECT_GE(TargetAlpha(220, white), 220);
}

TEST(Util_AdaptiveOpacity, SmoothingRisesFasterThanItFalls)
{
	const auto risen = StepAlpha(96, 255);
	const auto fallen = StepAlpha(255, 96);

	EXPECT_GT(risen, 96);
	EXPECT_LT(fallen, 255);
	EXPECT_GT(risen - 96, 255 - fallen);
	EXPECT_EQ(StepAlpha(128, 128), 128);
}

TEST(Util_AdaptiveOpacity, CachedAlphaChangesOnlyWhenSamplesAreUpdated)
{
	OpacityState state;
	const std::array white { Util::Color { 255, 255, 255 } };
	const std::array dark { Util::Color { 0, 0, 0 } };

	EXPECT_EQ(state.Value(96), 96);

	state.Update(96, white);
	const auto brightAlpha = state.Value(96);
	EXPECT_GT(brightAlpha, 96);

	// Reading the cached value cannot react to a changed background.
	EXPECT_EQ(state.Value(96), brightAlpha);

	state.Update(96, dark);
	EXPECT_LT(state.Value(96), brightAlpha);
}
