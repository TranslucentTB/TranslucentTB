#include "uwp.hpp"
#include <Windows.h>
#include <appmodel.h>
#include <winrt/Windows.System.h>

#include "../ProgramLog/error/winrt.hpp"

using namespace winrt::Windows;

bool UWP::HasPackageIdentity() noexcept
{
	UINT32 length = 0;
	const LONG result = GetCurrentPackageId(&length, nullptr);
	return result != APPMODEL_ERROR_NO_PACKAGE;
}

Foundation::IAsyncAction UWP::OpenUri(const Foundation::Uri &uri)
{
	bool opened;
	try
	{
		opened = co_await System::Launcher::LaunchUriAsync(uri);
	}
	catch (const winrt::hresult_error& err)
	{
		HresultErrorHandle(err, spdlog::level::err, L"Failed to launch uri.");
		co_return;
	}

	if (!opened)
	{
		MessagePrint(spdlog::level::err, L"Uri was not launched.");
	}
}
