#include "std.hpp"

#include "errno.hpp"
#include "win32.hpp"

std::wstring Error::MessageFromStdErrorCode(const std::error_code &err)
{
	if (err.category() == std::system_category())
	{
		return MessageFromHRESULT(HRESULT_FROM_WIN32(err.value()));
	}
	else if (err.category() == std::generic_category())
	{
		return MessageFromErrno(err.value());
	}
	else
	{
		return L"Unknown error";
	}
}
