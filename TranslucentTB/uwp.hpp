#pragma once
#include <ostream>
#include <string>
#include <string_view>
#include <winrt/base.h>

namespace UWP {
	enum class FolderType {
		Temporary,
		Roaming
	};

	winrt::hstring GetApplicationFolderPath(const FolderType &type);

	std::wstring GetApplicationVersion();
}

inline std::wostream &operator <<(std::wostream &stream, const winrt::hstring &str)
{
	return stream << static_cast<std::wstring_view>(str);
}