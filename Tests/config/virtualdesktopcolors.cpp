#include <gtest/gtest.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "config/config.hpp"
#include "undoc/virtualdesktop.hpp"

namespace {
	constexpr GUID First = { 0x12345678, 0xABCD, 0x1234, { 0xAB, 0xCD, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB } };
	constexpr GUID Other = { 0x23456789, 0xABCD, 0x1234, { 0xAB, 0xCD, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB } };
	Config Parse(const wchar_t *json)
	{
		rj::GenericDocument<rj::UTF16LE<>> document;
		document.Parse(json);
		if (document.HasParseError()) throw std::invalid_argument("Invalid test JSON");
		Config config;
		config.Deserialize(document);
		return config;
	}
}

TEST(VirtualDesktopColors, LegacyConfigurationDoesNotEnableOverrides)
{
	const auto config = Parse(LR"({"desktop_appearance":{"accent":"acrylic","color":"#123456FF"}})");
	EXPECT_FALSE(config.DesktopColors.Enabled);
	EXPECT_TRUE(config.DesktopColors.Colors.empty());
	EXPECT_FALSE(config.DesktopColors.Find(First));
	EXPECT_EQ(config.DesktopAppearance.Color, (Util::Color { 0x12, 0x34, 0x56, 0xFF }));
}

TEST(VirtualDesktopColors, RestrictsUndocumentedInterfaceLayoutToKnownBuilds)
{
	EXPECT_FALSE(VirtualDesktopAbi::IsSupportedBuild(19045, 9999));
	EXPECT_FALSE(VirtualDesktopAbi::IsSupportedBuild(22000, 9999));
	EXPECT_FALSE(VirtualDesktopAbi::IsSupportedBuild(22631, 9999));
	EXPECT_FALSE(VirtualDesktopAbi::IsSupportedBuild(26100, 2604));
	EXPECT_TRUE(VirtualDesktopAbi::IsSupportedBuild(26100, 2605));
	EXPECT_TRUE(VirtualDesktopAbi::IsSupportedBuild(26200, 9457));
	EXPECT_FALSE(VirtualDesktopAbi::IsSupportedBuild(26300, 0));
}

TEST(VirtualDesktopColors, NormalizesGuidAndMatchesOnlyAssignedIdentity)
{
	const auto config = Parse(LR"({"virtual_desktop_colors":{"enabled":true,"colors":{"{12345678-abcd-1234-abcd-1234567890ab}":"#2463EB80"}}})");
	ASSERT_TRUE(config.DesktopColors.Find(First));
	EXPECT_EQ(*config.DesktopColors.Find(First), (Util::Color { 0x24, 0x63, 0xEB, 0x80 }));
	EXPECT_FALSE(config.DesktopColors.Find(Other));
}

TEST(VirtualDesktopColors, DisabledAssignmentsAreRetainedAndNotApplied)
{
	const auto config = Parse(LR"({"virtual_desktop_colors":{"enabled":false,"colors":{"{12345678-ABCD-1234-ABCD-1234567890AB}":"#2463EBFF"}}})");
	EXPECT_EQ(config.DesktopColors.Colors.size(), 1);
	EXPECT_FALSE(config.DesktopColors.Find(First));
}

TEST(VirtualDesktopColors, RoundTripsAssignmentsAndAlpha)
{
	Config config;
	config.DesktopColors.Enabled = true;
	config.DesktopColors.Colors.emplace(VirtualDesktopColors::Key(First), Util::Color { 4, 5, 6, 128 });
	config.DesktopColors.Colors.emplace(VirtualDesktopColors::Key(Other), Util::Color { 7, 8, 9, 255 });
	rj::GenericStringBuffer<rj::UTF16LE<>> buffer;
	rj::Writer<decltype(buffer), rj::UTF16LE<>, rj::UTF16LE<>> writer(buffer);
	writer.StartObject();
	config.Serialize(writer);
	writer.EndObject();
	const auto loaded = Parse(buffer.GetString());
	EXPECT_TRUE(loaded.DesktopColors.Enabled);
	EXPECT_EQ(loaded.DesktopColors.Colors, config.DesktopColors.Colors);
}

TEST(VirtualDesktopColors, OverridesTintableStylesAndPreservesOtherAttributes)
{
	const Util::Color color { 1, 2, 3, 128 };
	for (const auto accent : { ACCENT_ENABLE_GRADIENT, ACCENT_ENABLE_TRANSPARENTGRADIENT, ACCENT_ENABLE_BLURBEHIND, ACCENT_ENABLE_ACRYLICBLURBEHIND })
	{
		const TaskbarAppearance input { accent, { 9, 8, 7, 6 }, false, true, 12.0f };
		const auto output = VirtualDesktopColors::Apply(input, color);
		EXPECT_EQ(output.Accent, accent);
		EXPECT_EQ(output.Color, color);
		EXPECT_EQ(output.ShowPeek, input.ShowPeek);
		EXPECT_EQ(output.ShowLine, input.ShowLine);
		EXPECT_EQ(output.BlurRadius, input.BlurRadius);
	}
}

TEST(VirtualDesktopColors, NormalUsesOpaqueOnlyWhenAColorIsAssigned)
{
	const TaskbarAppearance input { ACCENT_NORMAL, { 9, 8, 7, 6 }, false, true, 12.0f };
	const auto unchanged = VirtualDesktopColors::Apply(input, std::nullopt);
	EXPECT_EQ(unchanged.Accent, ACCENT_NORMAL);
	EXPECT_EQ(unchanged.Color, input.Color);
	const auto assigned = VirtualDesktopColors::Apply(input, Util::Color { 1, 2, 3, 255 });
	EXPECT_EQ(assigned.Accent, ACCENT_ENABLE_GRADIENT);
	EXPECT_EQ(assigned.Color, (Util::Color { 1, 2, 3, 255 }));
}

TEST(VirtualDesktopColors, RejectsInvalidAndDuplicateAssignments)
{
	for (const auto json : {
		LR"({"virtual_desktop_colors":{"colors":{"Desktop 1":"#FFFFFF"}}})",
		LR"({"virtual_desktop_colors":{"colors":{"{00000000-0000-0000-0000-000000000000}":"#FFFFFF"}}})",
		LR"({"virtual_desktop_colors":{"colors":{"{12345678-ABCD-1234-ABCD-1234567890AB}":"invalid"}}})",
		LR"({"virtual_desktop_colors":{"colors":{"{12345678-ABCD-1234-ABCD-1234567890AB}":123}}})",
		LR"({"virtual_desktop_colors":{"colors":{"{12345678-ABCD-1234-ABCD-1234567890AB}":"#FFFFFF","{12345678-abcd-1234-abcd-1234567890ab}":"#000000"}}})" })
		EXPECT_THROW(Parse(json), rjh::DeserializationError);
}
