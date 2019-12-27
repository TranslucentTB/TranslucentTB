#include "win32.hpp"
#include <fmt/format.h>
#include <type_traits>
#include <WinBase.h>
#include <winnt.h>
#include <wil/resource.h>

#include "util/strings.hpp"

std::wstring FormatHRESULT(HRESULT result, std::wstring_view description)
{
	return fmt::format(
		fmt(L"0x{:08X}: {}"),
		static_cast<std::make_unsigned_t<HRESULT>>(result), // needs this otherwise we get some error codes in the negatives
		description
	);
}

std::wstring FormatIRestrictedErrorInfo(HRESULT result, BSTR description)
{
	return FormatHRESULT(result, Util::Trim({ description, SysStringLen(description) }));
}

std::wstring Error::MessageFromHRESULT(HRESULT result)
{
	wil::unique_hlocal_string error;
	const DWORD count = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		nullptr,
		result,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		reinterpret_cast<wchar_t*>(error.put()),
		0,
		nullptr
	);

	return FormatHRESULT(result, count
		? Util::Trim({ error.get(), count })
		: fmt::format(fmt(L"[failed to get message for HRESULT] {}"), MessageFromHRESULT(HRESULT_FROM_WIN32(GetLastError()))));
}

std::wstring Error::MessageFromIRestrictedErrorInfo(IRestrictedErrorInfo *info)
{
	if (info)
	{
		HRESULT hr, errorCode;
		wil::unique_bstr description, restrictedDescription, capabilitySid;

		hr = info->GetErrorDetails(description.put(), &errorCode, restrictedDescription.put(), capabilitySid.put());
		if (SUCCEEDED(hr))
		{
			if (restrictedDescription)
			{
				return FormatIRestrictedErrorInfo(errorCode, restrictedDescription.get());
			}
			else if (description)
			{
				return FormatIRestrictedErrorInfo(errorCode, description.get());
			}
			else
			{
				return MessageFromHRESULT(errorCode);
			}
		}
		else
		{
			return fmt::format(fmt(L"[failed to get details from IRestrictedErrorInfo] {}"), MessageFromHRESULT(hr));
		}
	}
	else
	{
		return L"[IRestrictedErrorInfo was null]";
	}
}

std::wstring Error::MessageFromHresultError(const winrt::hresult_error &err)
{
	if (const auto info = err.try_as<IRestrictedErrorInfo>())
	{
		return MessageFromIRestrictedErrorInfo(info.get());
	}
	else
	{
		return MessageFromHRESULT(err.code());
	}
}
