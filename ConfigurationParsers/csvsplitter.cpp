#include "csvsplitter.hpp"
#include <algorithm>

#include "util/strings.hpp"

std::vector<std::wstring> CharacterSeparatedValuesSplitter::SplitLine(std::wstring_view line) const
{
	std::vector<std::wstring> values;

	bool isEscaped = false;
	for (wchar_t current : line)
	{
		if (current == m_Separator && !isEscaped)
		{
			isEscaped = false;
			if (!values.empty())
			{
				Util::TrimInplace(values.back());
			}
			else
			{
				values.push_back({ });
			}
			values.push_back({ });
		}
		else if (current == m_Escape && !isEscaped)
		{
			isEscaped = true;
		}
		else if (current == m_Comment && !isEscaped)
		{
			break;
		}
		else
		{
			if (values.empty())
			{
				values.push_back({ });
			}
			values.back() += current;
			isEscaped = false;
		}
	}

	// Trim last value
	if (!values.empty())
	{
		Util::TrimInplace(values.back());

		// If there is only 1 value and it's empty, return an empty vector.
		if (values.size() == 1 && values.back().empty())
		{
			values.clear();
		}
	}

	return values;
}
