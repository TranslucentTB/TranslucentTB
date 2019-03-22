#include "commentedkvpparser.hpp"
#include "util/strings.hpp"

std::pair<std::wstring, std::wstring> CommentedKeyValueParser::ParseLine(std::wstring_view line) const
{
	// pre-allocate
	std::wstring key(line.length(), L'\0');
	key.clear();

	std::wstring value(line.length(), L'\0');
	value.clear();

	bool isValue = false;
	bool isEscaped = false;
	for (wchar_t current : line)
	{
		if (current == m_Delimiter && !isEscaped)
		{
			if (!isValue)
			{
				isValue = true;
			}
			else
			{
				throw std::invalid_argument("Multiple instances of delimiter in this line");
			}
			isEscaped = false;
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
			(isValue ? value : key) += current;
			isEscaped = false;
		}
	}

	if (!isValue)
	{
		throw std::invalid_argument("No instances of delimiter in this line");
	}

	Util::TrimInplace(key);
	Util::TrimInplace(value);

	return { std::move(key), std::move(value) };
}
