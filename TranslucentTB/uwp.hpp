#pragma once
#include <string>
#include <ppltasks.h>
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

};