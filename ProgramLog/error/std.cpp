#include "std.hpp"

#include "win32.hpp"
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
		fmt::wmemory_buffer formatErrBuf;
		MessageFromErrno(formatErrBuf, strErr);

		err_buf.clear();
		fmt::format_to(err_buf, FMT_STRING(L"[failed to get message for errno_t] {}"), Util::ToStringView(formatErrBuf));
	}

	auto str = Util::ToStringView(err_buf);
	Util::TrimInplace(str);

	fmt::format_to(buf, FMT_STRING(L"{}: {}"), err, str);
}

void Error::MessageFromStdSystemError(fmt::wmemory_buffer &buf, const std::system_error &err)
{
	if (err.code().category() == std::system_category())
	{
		Error::MessageFromHRESULT(buf, HRESULT_FROM_WIN32(err.code().value()));
	}
	else if (err.code().category() == std::generic_category())
	{
		Error::MessageFromErrno(buf, err.code().value());
	}
	else
	{
		static constexpr std::wstring_view UNKNOWN_ERROR = L"Unknown error";
		buf.append(UNKNOWN_ERROR.data(), UNKNOWN_ERROR.data() + UNKNOWN_ERROR.length());
	}
}
