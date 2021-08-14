#include "std.hpp"

#include "errno.hpp"
#include "win32.hpp"

void Error::MessageFromStdErrorCode(fmt::wmemory_buffer &buf, const std::error_code &err)
{
	if (err.category() == std::system_category())
	{
		MessageFromHRESULT(buf, HRESULT_FROM_WIN32(err.value()));
	}
	else if (err.category() == std::generic_category())
	{
		MessageFromErrno(buf, err.value());
	}
	else
	{
		static constexpr std::wstring_view UNKNOWN_ERROR = L"Unknown error";
		buf.append(UNKNOWN_ERROR);
	}
}
