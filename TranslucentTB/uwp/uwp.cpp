#include "uwp.hpp"
#include <Windows.h>
#include <appmodel.h>
#include <DispatcherQueue.h>

#include "../ProgramLog/error/win32.hpp"
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

winrt::Windows::System::DispatcherQueueController UWP::CreateDispatcherController()
{
	const DispatcherQueueOptions options = {
		.dwSize = sizeof(options),
		.threadType = DQTYPE_THREAD_CURRENT,
		.apartmentType = DQTAT_COM_STA
	};

	winrt::com_ptr<ABI::Windows::System::IDispatcherQueueController> controller;
	HresultVerify(CreateDispatcherQueueController(options, controller.put()), spdlog::level::critical, L"Failed to create dispatcher!");
	return { controller.detach(), winrt::take_ownership_from_abi };
}
