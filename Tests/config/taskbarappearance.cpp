#include <gtest/gtest.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "config/taskbarappearance.hpp"
#include "../../Xaml/Models/Primitives/TaskbarAppearance.h"

namespace
{
	using encoding = rj::UTF16LE<>;
	using document = rj::GenericDocument<encoding>;
	using string_buffer = rj::GenericStringBuffer<encoding>;
	using writer = rj::Writer<string_buffer, encoding, encoding>;
}

TEST(Config_TaskbarAppearance, AdaptiveOpacityDefaultsToFalse)
{
	EXPECT_FALSE(TaskbarAppearance {}.AdaptiveOpacity);
}

TEST(Config_TaskbarAppearance, AdaptiveOpacityDeserializesFromJson)
{
	document json;
	json.Parse(LR"({"adaptive_opacity":true})");
	ASSERT_FALSE(json.HasParseError());

	TaskbarAppearance appearance;
	appearance.Deserialize(json, nullptr);

	EXPECT_TRUE(appearance.AdaptiveOpacity);
}

TEST(Config_TaskbarAppearance, AdaptiveOpacitySerializesToJson)
{
	TaskbarAppearance appearance;
	appearance.AdaptiveOpacity = true;

	string_buffer buffer;
	writer jsonWriter(buffer);
	jsonWriter.StartObject();
	appearance.Serialize(jsonWriter);
	jsonWriter.EndObject();

	document json;
	json.Parse(buffer.GetString());
	ASSERT_FALSE(json.HasParseError());
	ASSERT_TRUE(json.HasMember(L"adaptive_opacity"));
	EXPECT_TRUE(json[L"adaptive_opacity"].GetBool());
}

TEST(Config_TaskbarAppearance, AdaptiveOpacityDeserializesFromWinRT)
{
	const auto winrtAppearance = winrt::make<winrt::TranslucentTB::Xaml::Models::Primitives::implementation::TaskbarAppearance>(
		txmp::AccentState::Clear,
		winrt::Windows::UI::Color { 0x40, 0x10, 0x20, 0x30 },
		true,
		false,
		9.0f,
		true);
	const TaskbarAppearance restored(winrtAppearance);

	EXPECT_TRUE(restored.AdaptiveOpacity);
}
