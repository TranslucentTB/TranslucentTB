#include "errno.hpp"

#include "util/strings.hpp"

std::wstring Error::MessageFromErrno(errno_t err)
{
	std::wstring str;
	// fairly reasonable size so that most error messages fit within it.
	str.resize_and_overwrite(256, [err](wchar_t* data, std::size_t count) -> std::size_t
	{
		if (const errno_t strErr = _wcserror_s(data, count + 1, err); strErr == 0)
		{
			return wcslen(data);
		}
		else
		{
			return 0;
		}
	});

	if (!str.empty())
	{
		Util::TrimInplace(str);
	}
	else
	{
		str = L"[failed to get message for errno_t]";
	}

	return std::format(L"{}: {}", err, str);
}
