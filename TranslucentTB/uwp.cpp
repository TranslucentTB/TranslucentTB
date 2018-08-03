#ifdef STORE
#include "uwp.hpp"
#include <winrt/Windows.Storage.h>

winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::ApplicationModel::StartupTask> UWP::GetApplicationStartupTask()
{
	static auto task = (co_await winrt::Windows::ApplicationModel::StartupTask::GetForCurrentPackageAsync()).GetAt(0);
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