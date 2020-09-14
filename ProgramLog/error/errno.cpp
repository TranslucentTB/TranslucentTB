#include "errno.hpp"

#include "util/strings.hpp"
#include "util/to_string_view.hpp"

void Error::MessageFromErrno(fmt::wmemory_buffer &buf, errno_t err)
{
	fmt::wmemory_buffer err_buf;
	err_buf.resize(fmt::inline_buffer_size);

	if (const errno_t strErr = _wcserror_s(err_buf.data(), err_buf.size(), err); strErr == 0)
	{
		err_buf.resize(wcslen(err_buf.data()));
	}
	else
	{
		err_buf.clear();
		static constexpr std::wstring_view ERRNO_FAILED = L"[failed to get message for errno_t]";
		err_buf.append(ERRNO_FAILED.data(), ERRNO_FAILED.data() + ERRNO_FAILED.length());
	}

	auto str = Util::ToStringView(err_buf);
	Util::TrimInplace(str);

	fmt::format_to(buf, FMT_STRING(L"{}: {}"), err, str);
}
