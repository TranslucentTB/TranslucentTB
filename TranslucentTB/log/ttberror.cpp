#include "ttberror.hpp"
#include <comdef.h>
#include <exception>
#include <functional>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <wil/resource.h>
#include <winerror.h>
#include <WinUser.h>

#include "constants.hpp"
#include "ttblog.hpp"
#include "util/strings.hpp"
#include "../win32.hpp"
#include "window.hpp"

std::wstring Error::MessageFromHRESULT(HRESULT result)
{
	wil::unique_hlocal_string error;
	const DWORD count = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		nullptr,
		result,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		reinterpret_cast<wchar_t *>(error.put()),
		0,
		nullptr
	);

	return FormatHRESULT(result, count
		? Util::Trim({ error.get(), count })
		: fmt::format(L"[failed to get message for HRESULT] {}", MessageFromHRESULT(HRESULT_FROM_WIN32(GetLastError()))));
}

std::wstring Error::MessageFromIRestrictedErrorInfo(IRestrictedErrorInfo *info)
{
	if (info)
	{
		HRESULT hr, errorCode;
		wil::unique_bstr description, restrictedDescription, capabilitySid;

		hr = info->GetErrorDetails(&description, &errorCode, &restrictedDescription, &capabilitySid);
		if (SUCCEEDED(hr))
		{
			if (restrictedDescription)
			{
				return FormatIRestrictedErrorInfo(errorCode, restrictedDescription.get());
			}
			else if (description)
			{
				return FormatIRestrictedErrorInfo(errorCode, description.get());
			}
			else
			{
				return MessageFromHRESULT(errorCode);
			}
		}
		else
		{
			return fmt::format(L"[failed to get details from IRestrictedErrorInfo] {}", MessageFromHRESULT(hr));
		}
	}
	else
	{
		return L"[IRestrictedErrorInfo was null]";
	}
}

std::wstring Error::MessageFromHresultError(const winrt::hresult_error &err)
{
	if (const auto info = err.try_as<IRestrictedErrorInfo>())
	{
		return MessageFromIRestrictedErrorInfo(info.get());
	}
	else
	{
		return MessageFromHRESULT(err.code());
	}
}

std::wstring Error::FormatHRESULT(HRESULT result, std::wstring_view description)
{
	return fmt::format(
		L"{:#08x}: {}",
		static_cast<std::make_unsigned_t<HRESULT>>(result), // needs this otherwise we get some error codes in the negatives
		description
	);
}

std::wstring Error::FormatIRestrictedErrorInfo(HRESULT result, BSTR description)
{
	return FormatHRESULT(result, Util::Trim({ description, SysStringLen(description) }));
}

void Error::HandleCommon(spdlog::level::level_enum level, std::wstring_view message, std::wstring_view error_message, const char *file, int line, const char *function)
{
	std::wstring msg;

	if (level == spdlog::level::critical)
	{
		if (!error_message.empty())
		{
			msg = fmt::format(FATAL_ERROR_MESSAGE L"\n\n{}\n\n{}", message, error_message);
		}
		else
		{
			msg = fmt::format(FATAL_ERROR_MESSAGE L"\n\n{}", message);
		}

		MessageBox(Window::NullWindow, msg.c_str(), FATAL_ERROR_TITLE, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);

		// Calling abort() will generate a dialog box, but we already have our own.
		// Raising a fail-fast exception skips it but also allows WER to do its job.
		RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
	}
	else if (level == spdlog::level::err)
	{
		if (!error_message.empty())
		{
			msg = fmt::format(ERROR_MESSAGE L"\n\n{}\n\n{}", message, error_message);
		}
		else
		{
			msg = fmt::format(ERROR_MESSAGE L"\n\n{}", message);
		}

		std::thread([error = std::move(msg)]
		{
			MessageBox(Window::NullWindow, error.c_str(), ERROR_TITLE, MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
		}).detach();
	}

	if (!error_message.empty())
	{
		msg = fmt::format(L"{} ({})", message, error_message);
	}
	else
	{
		msg = message;
	}

	spdlog::log({ file, line, function }, level, msg);
}
