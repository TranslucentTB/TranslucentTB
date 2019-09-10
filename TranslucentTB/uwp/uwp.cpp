#include "uwp.hpp"
#include <winrt/Windows.ApplicationModel.Core.h>

using namespace winrt;
using namespace Windows::ApplicationModel;

Version UWP::GetApplicationVersion()
{
	static const auto version = Package::Current().Id().Version();

	return Version::FromPackageVersion(version);
}
