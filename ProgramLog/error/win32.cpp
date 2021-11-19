#include "win32.hpp"
#include <type_traits>
#include <WinBase.h>
#include <winnt.h>

#include "util/strings.hpp"

std::wstring Error::impl::FormatHRESULT(HRESULT result, std::wstring_view description)
{
	return std::format(
		L"0x{:08X}: {}",
		static_cast<std::make_unsigned_t<HRESULT>>(result), // needs this otherwise we get some error codes in the negatives
		description
	);
}

wil::unique_hlocal_string Error::impl::FormatMessageForLanguage(HRESULT result, DWORD langId, DWORD &count)
{
	wil::unique_hlocal_string error;
	count = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		result,
		langId,
		reinterpret_cast<wchar_t*>(error.put()),
		0,
		nullptr
	);

	return error;
}

std::wstring Error::MessageFromHRESULT(HRESULT result)
{
	DWORD count = 0;
	auto error = impl::FormatMessageForLanguage(result, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), count);
	DWORD lastErr = GetLastError();
	if (!count && (lastErr == ERROR_MUI_FILE_NOT_FOUND || lastErr == ERROR_RESOURCE_LANG_NOT_FOUND))
	{
		// US language not installed, try again with system language.
		error = impl::FormatMessageForLanguage(result, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), count);
		lastErr = GetLastError();
	}

	std::wstring errBuf;
	std::wstring_view description;
	if (count)
	{
		description = Util::Trim({ error.get(), count });
	}
	else
	{
		errBuf = std::format(L"[failed to get message] 0x{:08X}", static_cast<std::make_unsigned_t<HRESULT>>(HRESULT_FROM_WIN32(lastErr)));
		description = errBuf;
	}

	return impl::FormatHRESULT(result, description);
}
