#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "color.hpp"

namespace Util::AdaptiveOpacity
{
	inline constexpr double BrightStart = 0.45;
	inline constexpr double BrightFull = 0.85;
	inline constexpr double NearWhite = 0.80;
	inline constexpr double FullWhiteCoverage = 0.25;
	inline constexpr uint8_t RiseStep = 48;
	inline constexpr uint8_t FallStep = 20;

	inline double Linearize(uint8_t channel) noexcept
	{
		const double value = static_cast<double>(channel) / 255.0;
		return value <= 0.04045
			? value / 12.92
			: std::pow((value + 0.055) / 1.055, 2.4);
	}

	inline double RelativeLuminance(Util::Color color) noexcept
	{
		return
			0.2126 * Linearize(color.R) +
			0.7152 * Linearize(color.G) +
			0.0722 * Linearize(color.B);
	}

	inline double BrightnessPressure(std::span<const Util::Color> samples) noexcept
	{
		if (samples.empty())
		{
			return 0.0;
		}

		std::vector<double> luminances;
		luminances.reserve(samples.size());
		std::size_t nearWhiteCount = 0;

		for (const auto sample : samples)
		{
			const double luminance = RelativeLuminance(sample);
			luminances.push_back(luminance);
			if (luminance >= NearWhite)
			{
				++nearWhiteCount;
			}
		}

		std::ranges::sort(luminances);
		const std::size_t percentileIndex = (luminances.size() - 1) * 3 / 4;
		const double percentilePressure = std::clamp(
			(luminances[percentileIndex] - BrightStart) / (BrightFull - BrightStart),
			0.0,
			1.0);
		const double whiteCoverage = static_cast<double>(nearWhiteCount) / static_cast<double>(samples.size());
		const double coveragePressure = std::clamp(whiteCoverage / FullWhiteCoverage, 0.0, 1.0);
		const double pressure = std::max(percentilePressure, coveragePressure);

		return pressure * pressure * (3.0 - 2.0 * pressure);
	}

	inline uint8_t TargetAlpha(uint8_t baseAlpha, std::span<const Util::Color> samples) noexcept
	{
		const double pressure = BrightnessPressure(samples);
		const double alpha = static_cast<double>(baseAlpha) + (255.0 - baseAlpha) * pressure;
		return static_cast<uint8_t>(std::clamp(std::lround(alpha), static_cast<long>(baseAlpha), 255L));
	}

	inline uint8_t StepAlpha(uint8_t current, uint8_t target) noexcept
	{
		if (current < target)
		{
			return static_cast<uint8_t>(std::min<int>(target, current + RiseStep));
		}

		if (current > target)
		{
			return static_cast<uint8_t>(std::max<int>(target, current - FallStep));
		}

		return current;
	}

	class OpacityState
	{
	private:
		std::optional<uint8_t> m_Alpha;

	public:
		uint8_t Value(uint8_t baseAlpha) const noexcept
		{
			return std::max(baseAlpha, m_Alpha.value_or(baseAlpha));
		}

		void Update(uint8_t baseAlpha, std::span<const Util::Color> samples) noexcept
		{
			const uint8_t current = Value(baseAlpha);
			m_Alpha = StepAlpha(current, TargetAlpha(baseAlpha, samples));
		}
	};
}
