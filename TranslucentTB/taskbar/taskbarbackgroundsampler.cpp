#include "taskbarbackgroundsampler.hpp"

#include <wingdi.h>
#include <winuser.h>
#include <wil/resource.h>

std::optional<std::vector<Util::Color>> TaskbarBackgroundSampler::Sample(const RECT &taskbarRect, const RECT &monitorRect) noexcept
{
	const auto points = SamplePoints(taskbarRect, monitorRect);
	if (!points)
	{
		return std::nullopt;
	}

	const HDC screenDc = GetDC(nullptr);
	if (!screenDc)
	{
		return std::nullopt;
	}
	const auto releaseDc = wil::scope_exit([screenDc]
	{
		ReleaseDC(nullptr, screenDc);
	});

	std::vector<Util::Color> samples;
	samples.reserve(points->size());
	for (const auto point : *points)
	{
		const COLORREF color = GetPixel(screenDc, point.x, point.y);
		if (color != CLR_INVALID)
		{
			samples.emplace_back(GetRValue(color), GetGValue(color), GetBValue(color));
		}
	}

	if (samples.empty())
	{
		return std::nullopt;
	}

	return samples;
}
