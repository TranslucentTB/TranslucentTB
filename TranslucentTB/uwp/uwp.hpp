#pragma once
#include <string>
#include <string_view>
#include <winrt/base.h>

#include "../version.hpp"

namespace UWP {
	enum class FolderType {
		Temporary,
		Roaming
	};

	winrt::hstring GetApplicationFolderPath(FolderType type);
	Version GetApplicationVersion();
	bool HasPackageIdentity();
	void CopyToClipboard(std::wstring_view str);
};