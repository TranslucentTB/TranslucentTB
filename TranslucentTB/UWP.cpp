#ifdef STORE
#include "UWP.hpp"
#include <winrt/Windows.Storage.h>

winrt::Windows::ApplicationModel::StartupTask &UWP::GetApplicationStartupTask()
{
	namespace am = winrt::Windows::ApplicationModel;
	static am::StartupTask task = am::StartupTask::GetForCurrentPackageAsync().get().GetAt(0);
	return task;
}

std::wstring UWP::GetApplicationFolderPath(const FolderType &type)
{
	static auto application_data = winrt::Windows::Storage::ApplicationData::Current();

	switch (type)
	{
	case FolderType::Temporary:
		return application_data.TemporaryFolder().Path().c_str();

	case FolderType::Roaming:
		return application_data.RoamingFolder().Path().c_str();

	// Apparently we can cast any integer to an enum class, so yeah...
	default:
		throw std::exception("wtf are you doing");
	}
}

#endif // STORE