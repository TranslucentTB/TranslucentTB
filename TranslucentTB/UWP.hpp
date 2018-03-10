#pragma once
#ifdef STORE

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>

namespace UWP {

	using namespace winrt;
	using namespace winrt::Windows::ApplicationModel;
	using namespace winrt::Windows::Storage;

	StartupTask GetApplicationStartupTask()
	{
		static StartupTask task = StartupTask::GetForCurrentPackageAsync().get().GetAt(0);
		return task;
	}

	enum class FolderType {
		Temporary,
		Roaming
	};

	std::wstring GetApplicationFolderPath(const FolderType &type)
	{
		static ApplicationData application_data = ApplicationData::Current();

		switch (type)
		{
		case FolderType::Temporary:
			return application_data.TemporaryFolder().Path().c_str();

		case FolderType::Roaming:
			return application_data.RoamingFolder().Path().c_str();
		}
	}

}

#endif // STORE