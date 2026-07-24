#include <gtest/gtest.h>

#include "../../TranslucentTB/taskbar/taskbarbackgroundsampler.hpp"

namespace
{
	constexpr RECT Monitor { 0, 0, 1920, 1080 };
}

TEST(TaskbarBackgroundSampler, SamplesAboveBottomTaskbar)
{
	const auto points = TaskbarBackgroundSampler::SamplePoints({ 0, 1032, 1920, 1080 }, Monitor);
	ASSERT_TRUE(points);

	for (const auto point : *points)
	{
		EXPECT_GE(point.x, Monitor.left);
		EXPECT_LT(point.x, Monitor.right);
		EXPECT_LT(point.y, 1032);
		EXPECT_GE(point.y, Monitor.top);
	}
}

TEST(TaskbarBackgroundSampler, SamplesBelowTopTaskbar)
{
	const auto points = TaskbarBackgroundSampler::SamplePoints({ 0, 0, 1920, 48 }, Monitor);
	ASSERT_TRUE(points);

	for (const auto point : *points)
	{
		EXPECT_GT(point.y, 47);
		EXPECT_LT(point.y, Monitor.bottom);
	}
}

TEST(TaskbarBackgroundSampler, SamplesRightOfLeftTaskbar)
{
	const auto points = TaskbarBackgroundSampler::SamplePoints({ 0, 0, 48, 1080 }, Monitor);
	ASSERT_TRUE(points);

	for (const auto point : *points)
	{
		EXPECT_GT(point.x, 47);
		EXPECT_LT(point.x, Monitor.right);
	}
}

TEST(TaskbarBackgroundSampler, SamplesLeftOfRightTaskbar)
{
	const auto points = TaskbarBackgroundSampler::SamplePoints({ 1872, 0, 1920, 1080 }, Monitor);
	ASSERT_TRUE(points);

	for (const auto point : *points)
	{
		EXPECT_LT(point.x, 1872);
		EXPECT_GE(point.x, Monitor.left);
	}
}

TEST(TaskbarBackgroundSampler, RejectsEmptyOrOffMonitorRectangles)
{
	EXPECT_FALSE(TaskbarBackgroundSampler::SamplePoints({ 0, 0, 0, 0 }, Monitor));
	EXPECT_FALSE(TaskbarBackgroundSampler::SamplePoints({ 2000, 1100, 2100, 1200 }, Monitor));
}
