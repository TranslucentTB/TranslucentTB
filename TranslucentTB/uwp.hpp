#pragma once
#ifdef STORE

#include <string>
#include <winrt/Windows.ApplicationModel.h>

class UWP {

public:
	static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::ApplicationModel::StartupTask> GetApplicationStartupTask();

	enum class FolderType {
		Temporary,
		Roaming
	};

	static std::wstring GetApplicationFolderPath(const FolderType &type);

};

#endif // STORE