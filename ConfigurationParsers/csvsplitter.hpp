#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _PARSERS_DLL
#define PARSERS_EXPORT dllexport
#else
#define PARSERS_EXPORT dllimport
#endif

class CharacterSeparatedValuesSplitter {
protected:
	wchar_t m_Separator, m_Escape, m_Comment;

public:
	inline CharacterSeparatedValuesSplitter(wchar_t separator = L',', wchar_t escape = L'\\', wchar_t comment = L';') :
		m_Separator(separator), m_Escape(escape), m_Comment(comment)
	{
		if (separator == escape)
		{
			throw std::invalid_argument("Separator cannot be the same than the escape character");
		}
		else if (comment == separator)
		{
			throw std::invalid_argument("Comment cannot be the same than the separator character");
		}
		else if (comment == escape)
		{
			throw std::invalid_argument("Comment cannot be the same than the escape character");
		}
	}

	__declspec(PARSERS_EXPORT) std::vector<std::wstring> SplitLine(std::wstring_view line) const;
};