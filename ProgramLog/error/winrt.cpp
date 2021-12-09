#include "winrt.hpp"
#include <wil/resource.h>

#include "util/strings.hpp"
#include "win32.hpp"

std::wstring Error::impl::FormatIRestrictedErrorInfo(HRESULT result, BSTR description)
{
	return FormatHRESULT(result, Util::Trim({ description, SysStringLen(description) }));
}

bool Error::MessageFromIRestrictedErrorInfo(std::wstring &buf, IRestrictedErrorInfo *info, HRESULT failureCode)
{
	if (info)
	{
		HRESULT hr, errorCode;
		wil::unique_bstr description, restrictedDescription, capabilitySid;

		hr = info->GetErrorDetails(description.put(), &errorCode, restrictedDescription.put(), capabilitySid.put());
		if (SUCCEEDED(hr))
		{
			if (errorCode == failureCode)
			{
				if (restrictedDescription)
				{
					buf = impl::FormatIRestrictedErrorInfo(errorCode, restrictedDescription.get());
				}
				else if (description)
				{
					buf = impl::FormatIRestrictedErrorInfo(errorCode, description.get());
				}
				else
				{
					buf = MessageFromHRESULT(errorCode);
				}

				return true; // allow crash with error info
			}
			else
			{
				buf = std::format(L"[IRestrictedErrorInfo did not return expected HRESULT] expected: 0x{:08X}, actual: 0x{:08X}", failureCode, errorCode);
			}
		}
		else
		{
			buf = std::format(L"[failed to get details from IRestrictedErrorInfo] {}", MessageFromHRESULT(hr));
		}
	}
	else
	{
		buf = L"[IRestrictedErrorInfo was null]";
	}

	return false;
}

winrt::com_ptr<IRestrictedErrorInfo> Error::MessageFromHresultError(std::wstring &buf, const winrt::hresult_error &err, HRESULT *errCode)
{
	HRESULT code = err.code();
	if (errCode)
	{
		*errCode = code;
	}

	if (const auto info = err.try_as<IRestrictedErrorInfo>())
	{
		if (MessageFromIRestrictedErrorInfo(buf, info.get(), code))
		{
			return info;
		}
	}
	else
	{
		buf = MessageFromHRESULT(code);
	}

	return nullptr;
}
