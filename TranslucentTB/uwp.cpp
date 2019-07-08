#include "uwp.hpp"
#include <winrt/Windows.Storage.h>

winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::ApplicationModel::StartupTask> UWP::GetApplicationStartupTask()
{
	static const auto task = (co_await winrt::Windows::ApplicationModel::StartupTask::GetForCurrentPackageAsync()).GetAt(0);
	return task;
}

winrt::hstring UWP::GetApplicationFolderPath(const FolderType &type)
{
	static const auto application_data = winrt::Windows::Storage::ApplicationData::Current();

	switch (type)
	{
	case FolderType::Temporary:
		return application_data.TemporaryFolder().Path();

	case FolderType::Roaming:
		return application_data.RoamingFolder().Path();

	// Apparently we can cast any integer to an enum class, so yeah...
	default:
		throw std::invalid_argument("type was not one of the known values");
	}
}