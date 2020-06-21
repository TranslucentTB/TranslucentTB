#include "win32.hpp"
#include <fmt/format.h>
#include <type_traits>
#include <WinBase.h>
#include <winnt.h>
#include <wil/resource.h>

#include "util/strings.hpp"
#include "util/to_string_view.hpp"

void Error::impl::FormatHRESULT(fmt::wmemory_buffer &buf, HRESULT result, std::wstring_view description)
{
	fmt::format_to(
		buf,
		fmt(L"0x{:08X}: {}"),
		static_cast<std::make_unsigned_t<HRESULT>>(result), // needs this otherwise we get some error codes in the negatives
		description
	);
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

	impl::FormatHRESULT(buf, result, count ? Util::Trim({ error.get(), count }) : L"[failed to get message for HRESULT]");
}
