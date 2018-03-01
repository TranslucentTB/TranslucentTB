#pragma once
// Here be dragons

#define _HIDE_GLOBAL_ASYNC_STATUS
#include <Windows.ApplicationModel.h>
#include <Windows.Storage.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

#include "SynchronousOperation.hpp"
#include "ttberror.hpp"

namespace UWP {

	using namespace ABI::Windows;
	using namespace Microsoft::WRL;
	using namespace Microsoft::WRL::Wrappers;

	ComPtr<ApplicationModel::IStartupTask> GetApplicationStartupTask()
	{
		static ComPtr<ApplicationModel::IStartupTask> task;

		if (!task)
		{
			typedef Foundation::Collections::IVectorView<ApplicationModel::StartupTask *> StartupTasksVector, *StartupTasksVectorPtr;

			ComPtr<ApplicationModel::IStartupTaskStatics> startup_tasks_statics;
			if (!Error::Handle(Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_StartupTask).Get(), &startup_tasks_statics), Error::Level::Log, L"Activating IStartupTaskStatics instance failed."))
			{
				return nullptr;
			}

			ComPtr<Foundation::IAsyncOperation<StartupTasksVectorPtr>> operation;
			if (!Error::Handle(startup_tasks_statics->GetForCurrentPackageAsync(&operation), Error::Level::Log, L"Starting acquisition of package startup tasks failed."))
			{
				return nullptr;
			}

			// Fuck off async
			ComPtr<StartupTasksVector> package_tasks;
			if (!Error::Handle(SynchronousOperation<StartupTasksVectorPtr>(operation.Get()).GetResults(&package_tasks), Error::Level::Log, L"Acquiring package startup tasks failed."))
			{
				return nullptr;
			}

			if (!Error::Handle(package_tasks->GetAt(0, &task), Error::Level::Log, L"Getting first package startup task failed."))
			{
				return nullptr;
			}
		}

		return task;
	}

	enum class FolderType {
		Temporary,
		Roaming
	};

	HString GetApplicationFolderPath(const FolderType &type)
	{
		static ComPtr<Storage::IApplicationData> application_data;
		if (!application_data)
		{
			ComPtr<Storage::IApplicationDataStatics> data_statics;
			Error::Handle(Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &data_statics), Error::Level::Fatal, L"Could not acquire a IApplicationDataStatics interface!");

			Error::Handle(data_statics->get_Current(&application_data), Error::Level::Fatal, L"Could not get current application data!");
		}

		ComPtr<Storage::IStorageFolder> folder;
		HRESULT result;
		switch (type)
		{
		case FolderType::Temporary:
			result = application_data->get_TemporaryFolder(&folder);
			break;

		case FolderType::Roaming:
			result = application_data->get_RoamingFolder(&folder);
			break;
		}
		Error::Handle(result, Error::Level::Fatal, L"Could not get storage folder");

		ComPtr<Storage::IStorageItem> item;
		Error::Handle(folder.As(&item), Error::Level::Fatal, L"Could not cast IStorageFolder to IStorageItem");

		HString path;
		Error::Handle(item->get_Path(path.ReleaseAndGetAddressOf()), Error::Level::Fatal, L"Could not get path of storage folder");

		return path;
	}

}