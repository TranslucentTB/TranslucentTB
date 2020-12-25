#pragma once
#include "winrt.hpp"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>

namespace UWP {
	bool HasPackageIdentity() noexcept;
	wf::IAsyncAction OpenUri(const wf::Uri &uri);
	winrt::Windows::System::DispatcherQueueController CreateDispatcherController();
};
