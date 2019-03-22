#pragma once
#include "kvpparser.hpp"

class CommentedKeyValueParser : public KeyValueParser {
protected:
	wchar_t m_Comment;

public:
	inline CommentedKeyValueParser(wchar_t delimiter = L'=', wchar_t escape = L'\\', wchar_t comment = L';') :
		KeyValueParser(delimiter, escape), m_Comment(comment)
	{
		if (comment == delimiter)
		{
			throw std::invalid_argument("Comment cannot be the same than the delimiter character");
		}
		else if (comment == escape)
		{
			throw std::invalid_argument("Comment cannot be the same than the escape character");
		}
	}

	__declspec(PARSERS_EXPORT) std::pair<std::wstring, std::wstring> ParseLine(std::wstring_view line) const override;
};