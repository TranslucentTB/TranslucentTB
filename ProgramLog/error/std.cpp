#include "std.hpp"

#include "win32.hpp"
#include "util/strings.hpp"

std::wstring Error::MessageFromErrno(errno_t err)
{
	std::wstring str;
	str.resize(256);

	const errno_t strErr = _wcserror_s(str.data(), str.length(), err);
	if (strErr == 0)
	{
		str.resize(wcslen(str.c_str()));
		Util::TrimInplace(str);
	}
	else
	{
		str = fmt::format(fmt(L"[failed to get message for errno_t] {}"), MessageFromErrno(strErr));
	}

	return fmt::format(fmt(L"{}: {}"), err, str);
}

std::wstring Error::MessageFromStdSystemError(const std::system_error &err)
{
	if (err.code().category() == std::system_category())
	{
		return Error::MessageFromHRESULT(HRESULT_FROM_WIN32(err.code().value()));
	}
	else if (err.code().category() == std::generic_category())
	{
		return Error::MessageFromErrno(err.code().value());
	}
	else
	{
		return L"Unknown error";
	}
}
