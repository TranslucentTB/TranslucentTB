#include "errno.hpp"

#include "util/strings.hpp"

std::wstring Error::MessageFromErrno(errno_t err)
{
	std::wstring str;
	str.resize(256);

	if (const errno_t strErr = _wcserror_s(str.data(), str.size(), err); strErr == 0)
	{
		str.resize(wcslen(str.c_str()));
		Util::TrimInplace(str);
	}
	else
	{
		str = L"[failed to get message for errno_t]";
	}

	return std::format(L"{}: {}", err, str);
}
