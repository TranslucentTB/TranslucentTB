#include "winrt.hpp"
#include <wil/resource.h>

#include "util/strings.hpp"
#include "win32.hpp"

void Error::impl::FormatIRestrictedErrorInfo(fmt::wmemory_buffer &buf, HRESULT result, BSTR description)
{
	FormatHRESULT(buf, result, Util::Trim({ description, SysStringLen(description) }));
}

bool Error::MessageFromIRestrictedErrorInfo(fmt::wmemory_buffer &buf, IRestrictedErrorInfo *info, HRESULT failureCode)
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
					impl::FormatIRestrictedErrorInfo(buf, errorCode, restrictedDescription.get());
				}
				else if (description)
				{
					impl::FormatIRestrictedErrorInfo(buf, errorCode, description.get());
				}
				else
				{
					MessageFromHRESULT(buf, errorCode);
				}

				return true; // allow crash with error info
			}
			else
			{
				fmt::format_to(buf, FMT_STRING(L"[IRestrictedErrorInfo did not return expected HRESULT] expected: 0x{:08X}, actual: 0x{:08X}"), failureCode, errorCode);
			}
		}
		else
		{
			fmt::wmemory_buffer hrBuf;
			MessageFromHRESULT(hrBuf, hr);
			fmt::format_to(buf, FMT_STRING(L"[failed to get details from IRestrictedErrorInfo] {}"), Util::ToStringView(hrBuf));
		}
	}
	else
	{
		static constexpr std::wstring_view INFO_NULL = L"[IRestrictedErrorInfo was null]";
		buf.append(INFO_NULL.data(), INFO_NULL.data() + INFO_NULL.length());
	}

	return false;
}

winrt::com_ptr<IRestrictedErrorInfo> Error::MessageFromHresultError(fmt::wmemory_buffer &buf, const winrt::hresult_error &err, HRESULT *errCode)
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
		MessageFromHRESULT(buf, code);
	}

	return nullptr;
}
