#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include "winrt.hpp"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>

namespace UWP {
	std::optional<std::wstring> GetPackageFamilyName();
	std::optional<std::filesystem::path> GetAppStorageFolder();
	wf::IAsyncAction OpenUri(const wf::Uri &uri);
	winrt::Windows::System::DispatcherQueueController CreateDispatcherController();
};
