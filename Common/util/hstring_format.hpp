#pragma once
#include <cassert>
#include <format>
#include <intsafe.h>
#include <winrt/base.h>
#include <winstring.h>
#include <wil/resource.h>

#include "fixed_string.hpp"

namespace Util {
	template<fixed_wstring FormatString, typename... Args>
	winrt::hstring hstring_format(Args&&... args)
	{
		const auto sizetSize = std::formatted_size(FormatString, args...);
		uint32_t u32Size = 0;
		winrt::check_hresult(SizeTToUInt32(sizetSize, &u32Size));

		wil::unique_hstring_buffer bufHandle;
		wchar_t* buf = nullptr;
		winrt::check_hresult(WindowsPreallocateStringBuffer(u32Size, &buf, bufHandle.put()));

		const auto result = std::format_to_n(buf, u32Size, FormatString, args...);
		assert(result.size == u32Size);

		winrt::hstring str;
		winrt::check_hresult(WindowsPromoteStringBuffer(bufHandle.get(), reinterpret_cast<HSTRING *>(winrt::put_abi(str))));
		bufHandle.release(); // we shouldn't free the HSTRING_BUFFER if WindowsPromoteStringBuffer succeeded.
		return str;
	}
}
