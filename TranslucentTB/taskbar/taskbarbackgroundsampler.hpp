#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <vector>
#include "arch.h"
#include <windef.h>

#include "util/color.hpp"

class TaskbarBackgroundSampler
{
public:
	static constexpr std::size_t SampleCount = 48;
	static constexpr LONG StripOffset = 2;
	using sample_points = std::array<POINT, SampleCount>;

	static std::optional<sample_points> SamplePoints(const RECT &taskbarRect, const RECT &monitorRect) noexcept
	{
		const LONG taskbarWidth = taskbarRect.right - taskbarRect.left;
		const LONG taskbarHeight = taskbarRect.bottom - taskbarRect.top;
		const LONG monitorWidth = monitorRect.right - monitorRect.left;
		const LONG monitorHeight = monitorRect.bottom - monitorRect.top;
		if (taskbarWidth <= 0 || taskbarHeight <= 0 || monitorWidth <= 0 || monitorHeight <= 0)
		{
			return std::nullopt;
		}

		sample_points points;
		if (taskbarWidth >= taskbarHeight)
		{
			const LONG first = std::max(taskbarRect.left, monitorRect.left);
			const LONG last = std::min(taskbarRect.right, monitorRect.right) - 1;
			if (first > last)
			{
				return std::nullopt;
			}

			const LONG topDistance = std::abs(taskbarRect.top - monitorRect.top);
			const LONG bottomDistance = std::abs(monitorRect.bottom - taskbarRect.bottom);
			const LONG sampleY = topDistance <= bottomDistance
				? std::clamp(taskbarRect.bottom + StripOffset - 1, monitorRect.top, monitorRect.bottom - 1)
				: std::clamp(taskbarRect.top - StripOffset, monitorRect.top, monitorRect.bottom - 1);

			for (std::size_t index = 0; index < points.size(); ++index)
			{
				points[index] = {
					first + static_cast<LONG>((last - first) * index / (points.size() - 1)),
					sampleY
				};
			}
		}
		else
		{
			const LONG first = std::max(taskbarRect.top, monitorRect.top);
			const LONG last = std::min(taskbarRect.bottom, monitorRect.bottom) - 1;
			if (first > last)
			{
				return std::nullopt;
			}

			const LONG leftDistance = std::abs(taskbarRect.left - monitorRect.left);
			const LONG rightDistance = std::abs(monitorRect.right - taskbarRect.right);
			const LONG sampleX = leftDistance <= rightDistance
				? std::clamp(taskbarRect.right + StripOffset - 1, monitorRect.left, monitorRect.right - 1)
				: std::clamp(taskbarRect.left - StripOffset, monitorRect.left, monitorRect.right - 1);

			for (std::size_t index = 0; index < points.size(); ++index)
			{
				points[index] = {
					sampleX,
					first + static_cast<LONG>((last - first) * index / (points.size() - 1))
				};
			}
		}

		return points;
	}

	static std::optional<std::vector<Util::Color>> Sample(const RECT &taskbarRect, const RECT &monitorRect) noexcept;
};
