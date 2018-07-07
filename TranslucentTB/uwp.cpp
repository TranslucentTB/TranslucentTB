#ifdef STORE
#include "uwp.hpp"
#include <winrt/Windows.Storage.h>

const winrt::Windows::ApplicationModel::StartupTask &UWP::GetApplicationStartupTask()
{
	static const auto task = winrt::Windows::ApplicationModel::StartupTask::GetForCurrentPackageAsync().get().GetAt(0);
	return task;
}

std::wstring UWP::GetApplicationFolderPath(const FolderType &type)
{
	static const auto application_data = winrt::Windows::Storage::ApplicationData::Current();

	switch (type)
	{
	case FolderType::Temporary:
		return application_data.TemporaryFolder().Path().c_str();

	case FolderType::Roaming:
		return application_data.RoamingFolder().Path().c_str();

	// Apparently we can cast any integer to an enum class, so yeah...
	default:
		throw std::invalid_argument("type was not one of the known values");
	}
}

#endif // STORE