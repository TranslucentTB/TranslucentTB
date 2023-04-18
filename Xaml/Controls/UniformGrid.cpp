#include "pch.h"
#include <numeric>
#include <ranges>

#include "UniformGrid.h"
#if __has_include("Controls/UniformGrid.g.cpp")
#include "Controls/UniformGrid.g.cpp"
#endif

#include "win32.hpp"

namespace winrt::TranslucentTB::Xaml::Controls::implementation
{
	wux::DependencyProperty UniformGrid::m_AutoLayoutProperty =
		wux::DependencyProperty::RegisterAttached(
			L"AutoLayout",
			xaml_typename<wf::IReference<bool>>(),
			xaml_typename<class_type>(),
			nullptr);

	void UniformGrid::OnDependencyPropertyChanged(const IInspectable &sender, const wux::DependencyPropertyChangedEventArgs &)
	{
		if (const auto that = sender.try_as<UniformGrid>())
		{
			that->InvalidateMeasure();
		}
	}

	wf::Size UniformGrid::MeasureOverride(const wf::Size &availableSize)
	{
		const auto visible = Children()
			| std::views::transform([](const wux::UIElement& element) noexcept { return element.try_as<wux::FrameworkElement>(); })
			| std::views::filter([](const wux::FrameworkElement& element) { return element && element.Visibility() != wux::Visibility::Collapsed; })
			| std::ranges::to<std::vector>();

		const auto [rows, columns] = GetDimensions(visible, Rows(), Columns(), FirstColumn());

		// Now that we know size, setup automatic rows/columns
		// to utilize Grid for UniformGrid behavior.
		// We also interleave any specified rows/columns with fixed sizes.
		SetupRowDefinitions(static_cast<uint32_t>(rows));
		SetupColumnDefinitions(static_cast<uint32_t>(columns));

		m_TakenSpots.clear();
		m_TakenSpots.resize(rows * columns, false);
		m_SpotsHeight = rows;
		m_SpotsWidth = columns;

		// Figure out which children we should automatically layout and where available openings are.
		for (const auto &child : visible)
		{
			const auto autoLayout = GetAutoLayout(child);
			if (!autoLayout)
			{
				// If an element needs to be forced in the 0, 0 position,
				// they should manually set UniformGrid.AutoLayout to False for that element.
				const auto row = composable_base::GetRow(child);
				const auto col = composable_base::GetColumn(child);

				if (row == 0 && col == 0)
				{
					SetAutoLayout(child, true);
				}
				else
				{
					SetAutoLayout(child, false);
					FillSpots(true, row, col, composable_base::GetColumnSpan(child), composable_base::GetRowSpan(child));
				}
			}
		}

		// Setup available size with our known dimensions now.
		// UniformGrid expands size based on largest singular item.
		const float columnSpacingSize = static_cast<float>(ColumnSpacing()) * (columns - 1);
		const float rowSpacingSize = static_cast<float>(RowSpacing()) * (rows - 1);

		const wf::Size childSize = {
			(availableSize.Width - columnSpacingSize) / columns,
			(availableSize.Height - rowSpacingSize) / rows
		};

		float maxWidth = 0.0;
		float maxHeight = 0.0;

		// Set Grid Row/Col for every child with autolayout = true
		// Backwards with FlowDirection
		auto freespots = GetFreeSpots(FirstColumn(), Orientation() == wuxc::Orientation::Vertical);
		auto freespot = freespots.begin();
		for (const auto &child : visible)
		{
			// Set location if we're in charge
			const auto autoLayout = GetAutoLayout(child);
			if (autoLayout && autoLayout.Value())
			{
				if (freespot != freespots.end())
				{
					const auto [row, column] = *freespot;

					composable_base::SetRow(child, row);
					composable_base::SetColumn(child, column);

					const auto rowspan = composable_base::GetRowSpan(child);
					const auto colspan = composable_base::GetColumnSpan(child);

					if (rowspan > 1 || colspan > 1)
					{
						// TODO: Need to tie this into iterator
						FillSpots(true, row, column, colspan, rowspan);
					}

					++freespot;
				}
				else
				{
					// We've run out of spots as the developer has
					// most likely given us a fixed size and too many elements
					// Therefore, tell this element it has no size and move on.
					child.Measure(wf::Size { });

					m_Overflow.push_back(child);

					continue;
				}
			}
			else
			{
				const auto row = composable_base::GetRow(child);
				const auto column = composable_base::GetColumn(child);
				if (row < 0 || row >= rows || column < 0 || column >= columns)
				{
					// A child is specifying a location, but that location is outside
					// of our grid space, so we should hide it instead.
					child.Measure(wf::Size { });

					m_Overflow.push_back(child);

					continue;
				}
			}

			// Get measurement for max child
			child.Measure(childSize);

			const auto desiredSize = child.DesiredSize();
			maxWidth = std::max(desiredSize.Width, maxWidth);
			maxHeight = std::max(desiredSize.Height, maxHeight);
		}

		// Return our desired size based on the largest child we found, our dimensions, and spacing.
		wf::Size desiredSize = {
			(maxWidth * columns) + columnSpacingSize,
			(maxHeight * rows) + rowSpacingSize
		};

		// Required to perform regular grid measurement, but ignore result.
		base_type::MeasureOverride(desiredSize);

		return desiredSize;
	}

	wf::Size UniformGrid::ArrangeOverride(const wf::Size &finalSize)
	{
		// Have grid to the bulk of our heavy lifting.
		const auto size = base_type::ArrangeOverride(finalSize);

		// Make sure all overflown elements have no size.
		for (const auto &child : m_Overflow)
		{
			child.Arrange(wf::Rect { });
		}

		m_Overflow.clear(); // Reset for next time.

		return size;
	}

	std::tuple<int, int> UniformGrid::GetDimensions(const std::vector<wux::FrameworkElement> &visible, int rows, int cols, int firstColumn)
	{
		// If a dimension isn't specified, we need to figure out the other one (or both).
		if (rows == 0 || cols == 0)
		{
			// Calculate the size & area of all objects in the grid to know how much space we need.
			const auto sizes = visible | std::views::transform([](const wux::FrameworkElement &item)
			{
				return composable_base::GetRowSpan(item) * composable_base::GetColumnSpan(item);
			});

			auto count = std::max(1, std::accumulate(sizes.begin(), sizes.end(), 0));

			if (rows == 0)
			{
				if (cols > 0)
				{
					// Bound check
					const auto first = (firstColumn >= cols || firstColumn < 0) ? 0 : firstColumn;

					// If we have columns but no rows, calculate rows based on column offset and number of children.
					rows = (count + first + (cols - 1)) / cols;
					return { rows, cols };
				}
				else
				{
					// Otherwise, determine square layout if both are zero.
					const auto size = static_cast<int>(std::ceil(std::sqrt(count)));

					// Figure out if firstColumn is in bounds
					const auto first = (firstColumn >= size || firstColumn < 0) ? 0 : firstColumn;

					rows = static_cast<int>(std::ceil(std::sqrt(count + first)));
					return { rows, rows };
				}
			}
			else if (cols == 0)
			{
				// If we have rows and no columns, then calculate columns needed based on rows
				cols = (count + (rows - 1)) / rows;

				// Now that we know a rough size of our shape, see if the FirstColumn effects that:
				const auto first = (firstColumn >= cols || firstColumn < 0) ? 0 : firstColumn;

				cols = (count + first + (rows - 1)) / rows;
			}
		}

		return { rows, cols };
	}

	void UniformGrid::SetupRowDefinitions(uint32_t rows)
	{
		const auto definitions = RowDefinitions();

		// Mark initial definitions so we don't erase them.
		for (const auto &rd : definitions)
		{
			if (!GetAutoLayout(rd))
			{
				SetAutoLayout(rd, false);
			}
		}

		// Remove non-autolayout rows we've added and then add them in the right spots.
		if (rows != definitions.Size())
		{
			for (int32_t r = definitions.Size() - 1; r >= 0; r--)
			{
				const auto layout = GetAutoLayout(definitions.GetAt(r));
				if (layout && layout.Value())
				{
					definitions.RemoveAt(r);
				}
			}

			for (uint32_t r = definitions.Size(); r < rows; r++)
			{
				wux::Controls::RowDefinition rd;
				SetAutoLayout(rd, true);
				definitions.InsertAt(r, rd);
			}
		}
	}

	void UniformGrid::SetupColumnDefinitions(uint32_t columns)
	{
		const auto definitions = ColumnDefinitions();

		// Mark initial definitions so we don't erase them.
		for (const auto &cd : definitions)
		{
			if (!GetAutoLayout(cd))
			{
				SetAutoLayout(cd, false);
			}
		}

		// Remove non-autolayout rows we've added and then add them in the right spots.
		if (columns != definitions.Size())
		{
			for (int32_t c = definitions.Size() - 1; c >= 0; c--)
			{
				const auto layout = GetAutoLayout(definitions.GetAt(c));
				if (layout && layout.Value())
				{
					definitions.RemoveAt(c);
				}
			}

			for (uint32_t c = definitions.Size(); c < columns; c++)
			{
				wux::Controls::ColumnDefinition cd;
				SetAutoLayout(cd, true);
				definitions.InsertAt(c, cd);
			}
		}
	}

	bool UniformGrid::GetSpot(int i, int j)
	{
		return m_TakenSpots[(i * m_SpotsWidth) + j];
	}

	void UniformGrid::FillSpots(bool value, int row, int column, int width, int height)
	{
		RECT rect1 = { 0, 0, m_SpotsWidth, m_SpotsHeight }, rect2 = { column, row, column + width, row + height };

		// Precompute bounds to skip branching in main loop
		RECT bounds { };
		IntersectRect(&bounds, &rect1, &rect2);

		for (int i = bounds.top; i < bounds.bottom; i++)
		{
			for (int j = bounds.left; j < bounds.right; j++)
			{
				m_TakenSpots[(i * m_SpotsWidth) + j] = value;
			}
		}
	}

	std::experimental::generator<std::tuple<int, int>> UniformGrid::GetFreeSpots(int firstColumn, bool topDown)
	{
		// Provides the next spot in the boolean array with a 'false' value.
		if (topDown)
		{
			// Layout spots from Top-Bottom, Left-Right (right-left handled automatically by Grid with Flow-Direction).
			// Effectively transpose the Grid Layout.
			for (int c = 0; c < m_SpotsWidth; c++)
			{
				int start = (c == 0 && firstColumn > 0 && firstColumn < m_SpotsHeight) ? firstColumn : 0;
				for (int r = start; r < m_SpotsHeight; r++)
				{
					if (!GetSpot(r, c))
					{
						co_yield { r, c };
					}
				}
			}
		}
		else
		{
			// Layout spots as normal from Left-Right.
			// (right-left handled automatically by Grid with Flow-Direction
			// during its layout, internal model is always left-right).
			for (int r = 0; r < m_SpotsHeight; r++)
			{
				int start = (r == 0 && firstColumn > 0 && firstColumn < m_SpotsWidth) ? firstColumn : 0;
				for (int c = start; c < m_SpotsWidth; c++)
				{
					if (!GetSpot(r, c))
					{
						co_yield { r, c };
					}
				}
			}
		}
	}
}
