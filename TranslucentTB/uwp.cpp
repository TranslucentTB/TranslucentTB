#include "uwp.hpp"
#include <sstream>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>

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