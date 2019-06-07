#pragma once
#include <ostream>
#include <string>
#include <string_view>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.h>

class UWP {
private:
	inline static winrt::Windows::ApplicationModel::Package GetCurrentPackage()
	{
		static const auto package = winrt::Windows::ApplicationModel::Package::Current();
		return package;
	}

public:
	enum class FolderType {
		Temporary,
		Roaming
	};

	static winrt::hstring GetApplicationFolderPath(FolderType type);

	static std::wstring GetApplicationVersion();

	static bool HasPackageIdentity();

	static winrt::Windows::Foundation::IAsyncOperation<bool> RelaunchApplication();
};

inline std::wostream &operator <<(std::wostream &stream, const winrt::hstring &str)
{
	return stream << static_cast<std::wstring_view>(str);
}