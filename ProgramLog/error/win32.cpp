#include "win32.hpp"
#include <fmt/format.h>
#include <type_traits>
#include <WinBase.h>
#include <winnt.h>

#include "util/strings.hpp"
#include "util/to_string_view.hpp"

void Error::impl::FormatHRESULT(fmt::wmemory_buffer &buf, HRESULT result, std::wstring_view description)
{
	fmt::format_to(
		buf,
		FMT_STRING(L"0x{:08X}: {}"),
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

void Error::MessageFromHRESULT(fmt::wmemory_buffer &buf, HRESULT result)
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

	fmt::wmemory_buffer errBuf;
	std::wstring_view description;
	if (count)
	{
		description = Util::Trim({ error.get(), count });
	}
	else
	{
		fmt::format_to(errBuf, FMT_STRING(L"[failed to get message] 0x{:08X}"), static_cast<std::make_unsigned_t<HRESULT>>(HRESULT_FROM_WIN32(lastErr)));
		description = Util::ToStringView(errBuf);
	}

	impl::FormatHRESULT(buf, result, description);
}
