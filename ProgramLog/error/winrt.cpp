#include "winrt.hpp"
#include <wil/resource.h>

#include "util/strings.hpp"
#include "win32.hpp"

std::wstring Error::impl::FormatIRestrictedErrorInfo(HRESULT result, BSTR description)
{
	return FormatHRESULT(result, Util::Trim({ description, SysStringLen(description) }));
}

std::wstring Error::MessageFromIRestrictedErrorInfo(IRestrictedErrorInfo *info, HRESULT errCode)
{
	if (info)
	{
		wil::unique_bstr description, restrictedDescription, capabilitySid;
		if (SUCCEEDED(info->GetErrorDetails(description.put(), &errCode, restrictedDescription.put(), capabilitySid.put())))
		{
			if (restrictedDescription)
			{
				return impl::FormatIRestrictedErrorInfo(errCode, restrictedDescription.get());
			}
			else if (description)
			{
				return impl::FormatIRestrictedErrorInfo(errCode, description.get());
			}

			// fall down to the return below, to call MessageFromHRESULT with the error code extracted from the error info.
		}
	}

	return MessageFromHRESULT(errCode);
}

std::wstring Error::MessageFromHresultError(const winrt::hresult_error &error)
{
	return MessageFromIRestrictedErrorInfo(error.try_as<IRestrictedErrorInfo>().get(), error.code());
}
