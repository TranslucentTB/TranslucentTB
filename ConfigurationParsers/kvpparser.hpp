#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#ifdef _PARSERS_DLL
#define PARSERS_EXPORT dllexport
#else
#define PARSERS_EXPORT dllimport
#endif

class KeyValueParser {
protected:
	wchar_t m_Delimiter, m_Escape, m_Comment;

public:
	inline KeyValueParser(wchar_t delimiter = L'=', wchar_t escape = L'\\', wchar_t comment = L';') :
		m_Delimiter(delimiter), m_Escape(escape), m_Comment(comment)
	{
		if (delimiter == escape)
		{
			throw std::invalid_argument("Delimiter cannot be the same than the escape character");
		}
		else if (comment == delimiter)
		{
			throw std::invalid_argument("Comment cannot be the same than the delimiter character");
		}
		else if (comment == escape)
		{
			throw std::invalid_argument("Comment cannot be the same than the escape character");
		}
	}

	__declspec(PARSERS_EXPORT) std::pair<std::wstring, std::wstring> ParseLine(std::wstring_view line) const;
};