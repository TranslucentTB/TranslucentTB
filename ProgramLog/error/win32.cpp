#include "win32.hpp"
#include <fmt/format.h>
#include <type_traits>
#include <WinBase.h>
#include <winnt.h>
#include <wil/resource.h>

#include "util/strings.hpp"
#include "util/to_string_view.hpp"

void FormatHRESULT(fmt::wmemory_buffer &buf, HRESULT result, std::wstring_view description)
{
	fmt::format_to(
		buf,
		fmt(L"0x{:08X}: {}"),
		static_cast<std::make_unsigned_t<HRESULT>>(result), // needs this otherwise we get some error codes in the negatives
		description
	);
}

void FormatIRestrictedErrorInfo(fmt::wmemory_buffer &buf, HRESULT result, BSTR description)
{
	FormatHRESULT(buf, result, Util::Trim({ description, SysStringLen(description) }));
}

void Error::MessageFromHRESULT(fmt::wmemory_buffer &buf, HRESULT result)
{
	wil::unique_hlocal_string error;
	const DWORD count = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		result,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		reinterpret_cast<wchar_t *>(error.put()),
		0,
		nullptr
	);

	if (count)
	{
		FormatHRESULT(buf, result, Util::Trim({ error.get(), count }));
	}
	else
	{
		fmt::wmemory_buffer hrBuf;
		MessageFromHRESULT(hrBuf, HRESULT_FROM_WIN32(GetLastError()));

		fmt::wmemory_buffer errBuf;
		fmt::format_to(errBuf, fmt(L"[failed to get message for HRESULT] {}"), Util::ToStringView(hrBuf));

		FormatHRESULT(buf, result, Util::ToStringView(errBuf));
	}
}

void Error::MessageFromIRestrictedErrorInfo(fmt::wmemory_buffer &buf, IRestrictedErrorInfo *info)
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
				FormatIRestrictedErrorInfo(buf, errorCode, restrictedDescription.get());
			}
			else if (description)
			{
				FormatIRestrictedErrorInfo(buf, errorCode, description.get());
			}
			else
			{
				MessageFromHRESULT(buf, errorCode);
			}
		}
		else
		{
			fmt::wmemory_buffer hrBuf;
			MessageFromHRESULT(hrBuf, hr);
			fmt::format_to(buf, fmt(L"[failed to get details from IRestrictedErrorInfo] {}"), Util::ToStringView(hrBuf));
		}
	}
	else
	{
		static constexpr std::wstring_view INFO_NULL = L"[IRestrictedErrorInfo was null]";
		buf.append(INFO_NULL.data(), INFO_NULL.data() + INFO_NULL.length());
	}
}
