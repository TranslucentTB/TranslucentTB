#pragma once
#include <ostream>
#include <ppltasks.h>
#include <string>
#include <winrt/Windows.ApplicationModel.h>

class UWP {
public:
	// task requires a default constructible value. StartupTask isn't, but a pointer to it is.
	static concurrency::task<const winrt::Windows::ApplicationModel::StartupTask *> GetApplicationStartupTask();

	enum class FolderType {
		Temporary,
		Roaming
	};

	static winrt::hstring GetApplicationFolderPath(const FolderType &type);

	static std::wstring GetApplicationVersion();
};

inline std::wostream &operator <<(std::wostream &stream, const winrt::hstring &str)
{
	return stream << str.c_str();
}