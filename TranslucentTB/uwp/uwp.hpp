#pragma once
#include "winrt.hpp"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>

namespace UWP {
	bool HasPackageIdentity() noexcept;
	winrt::Windows::Foundation::IAsyncAction OpenUri(const winrt::Windows::Foundation::Uri &uri);
	winrt::Windows::System::DispatcherQueueController CreateDispatcherController();
};
