#include "uwp.hpp"
#include <sstream>
#include <winrt/Windows.Storage.h>

concurrency::task<const winrt::Windows::ApplicationModel::StartupTask *> UWP::GetApplicationStartupTask()
{
	return concurrency::create_task([]() -> const winrt::Windows::ApplicationModel::StartupTask *
	{
		static const auto task = winrt::Windows::ApplicationModel::StartupTask::GetForCurrentPackageAsync().get().GetAt(0);
		return &task;
	});
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

	default:
		throw std::invalid_argument("type was not one of the known values");
	}
}

std::wstring UWP::GetApplicationVersion()
{
	static const auto version = winrt::Windows::ApplicationModel::Package::Current().Id().Version();

	std::wostringstream str;
	str << version.Major << L'.' << version.Minor << L'.' << version.Revision << L'.' << version.Build;
	return str.str();
}