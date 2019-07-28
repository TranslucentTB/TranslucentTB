#include "uwp.hpp"
#include "arch.h"
#include <cstdint>
#include <fmt/format.h>
#include <ShlObj.h>
#include <appmodel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.h>

using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace Windows::Storage;

hstring UWP::GetApplicationFolderPath(FolderType type)
{
	static const auto application_data = ApplicationData::Current();

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
	static const auto version = Package::Current().Id().Version();
;
	return fmt::format(fmt(L"{}.{}.{}.{}"), version.Major, version.Minor, version.Revision, version.Build);
}

bool UWP::HasPackageIdentity()
{
	static const bool has_identity = []
	{
		uint32_t length = 0;
		const auto result = GetCurrentPackageFamilyName(&length, nullptr);
		return result != APPMODEL_ERROR_NO_PACKAGE;
	}();

	return has_identity;
}

void UWP::CopyToClipboard(std::wstring_view str)
{
	DataTransfer::DataPackage package;
	package.SetText(str);

	DataTransfer::Clipboard::SetContent(package);
}
