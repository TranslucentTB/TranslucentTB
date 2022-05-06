#include "winrt.hpp"
#include <wil/resource.h>

#include "util/strings.hpp"
#include "win32.hpp"

std::wstring Error::impl::FormatIRestrictedErrorInfo(HRESULT result, BSTR description)
{
	return FormatHRESULT(result, Util::Trim({ description, SysStringLen(description) }));
}

bool Error::MessageFromIRestrictedErrorInfo(std::wstring &buf, IRestrictedErrorInfo *info, HRESULT *errCode)
{
	HRESULT hr;
	wil::unique_bstr description, restrictedDescription, capabilitySid;

	hr = info->GetErrorDetails(description.put(), errCode, restrictedDescription.put(), capabilitySid.put());
	if (SUCCEEDED(hr))
	{
		if (restrictedDescription)
		{
			buf = impl::FormatIRestrictedErrorInfo(*errCode, restrictedDescription.get());
		}
		else if (description)
		{
			buf = impl::FormatIRestrictedErrorInfo(*errCode, description.get());
		}
		else
		{
			buf = MessageFromHRESULT(*errCode);
		}

		return true;
	}
	else
	{
		buf = std::format(L"[failed to get details from IRestrictedErrorInfo] {}", MessageFromHRESULT(hr));
		return false;
	}
}

winrt::com_ptr<IRestrictedErrorInfo> Error::MessageFromHresultError(std::wstring &buf, const winrt::hresult_error &err, HRESULT *errCode)
{
	if (const auto info = err.try_as<IRestrictedErrorInfo>())
	{
		if (!MessageFromIRestrictedErrorInfo(buf, info.get(), errCode))
		{
			*errCode = err.code();
		}

		return info;
	}
	else
	{
		*errCode = err.code();
		buf = MessageFromHRESULT(*errCode);

		return nullptr;
	}
}
