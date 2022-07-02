#include "winrt.hpp"
#include <intrin.h>
#include <roerrorapi.h>
#include <wil/resource.h>
#include <winnt.h>

#include "util/strings.hpp"
#include "win32.hpp"

std::wstring Error::impl::FormatIRestrictedErrorInfo(HRESULT result, BSTR description)
{
	return FormatHRESULT(result, Util::Trim({ description, SysStringLen(description) }));
}

void Error::impl::HandleCriticalWithErrorInfo(std::wstring_view message, std::wstring_view error_message, std::source_location location, HRESULT err, IRestrictedErrorInfo* errInfo)
{
	HandleCriticalCommon(message, error_message, location);

	if (errInfo)
	{
		if (const HRESULT hr = SetRestrictedErrorInfo(errInfo); SUCCEEDED(hr))
		{
			// This gives much better error reporting if the error came from a WinRT module:
			// the stack trace in the dump, debugger and telemetry is unaffected by our error handling,
			// giving us better insight into what went wrong.
			RoFailFastWithErrorContext(err);
		}
	}

	__fastfail(FAST_FAIL_FATAL_APP_EXIT);
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
