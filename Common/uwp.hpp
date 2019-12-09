#pragma once
#include "arch.h"
#include <windef.h>
#include <apisetcconv.h>
#include <appmodel.h>
#include <cstdint>
#include <string_view>
#include <winerror.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

namespace UWP {
	inline bool HasPackageIdentity() noexcept
	{
		UINT32 length = 0;
		const LONG result = GetCurrentPackageFamilyName(&length, nullptr);
		return result != APPMODEL_ERROR_NO_PACKAGE;
	}

	inline void CopyToClipboard(std::wstring_view str)
	{
		using namespace winrt::Windows::ApplicationModel::DataTransfer;

		DataPackage package;
		package.SetText(str);

		Clipboard::SetContent(package);
	}
}
