#include "ttberror.hpp"
#include <comdef.h>
#include <exception>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <winerror.h>
#include <WinUser.h>

#include "autofree.hpp"
#include "common.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#include "win32.hpp"
#include "window.hpp"

bool Error::Handle(HRESULT error, Level level, const wchar_t *message, const wchar_t *file, int line, const wchar_t *function)
{
	if (FAILED(error))
	{
		const std::wstring error_message = ExceptionFromHRESULT(error);
		std::wostringstream boxbuffer;
		if (level != Level::Log && level != Level::Debug)
		{
			boxbuffer << message << L"\n\n";

			if (level == Level::Fatal)
			{
				boxbuffer << L"Program will exit.\n\n";
			}

			boxbuffer << error_message;
		}

		std::wostringstream err;
		err << message << L' ' << error_message <<
			L" (" << file << L':' << line << L" at function " << function << L')';

		switch (level)
		{
		case Level::Debug:
			err << L'\n';
			OutputDebugString(err.str().c_str());
			break;
		case Level::Log:
			Log::OutputMessage(err.str());
			break;
		case Level::Error:
			Log::OutputMessage(err.str());
			std::thread([str = boxbuffer.str()]
			{
				MessageBox(Window::NullWindow, str.c_str(), NAME L" - Error", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
			}).detach();
			break;
		case Level::Fatal:
			Log::OutputMessage(err.str());
			MessageBox(Window::NullWindow, boxbuffer.str().c_str(), NAME L" - Fatal error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
			RaiseFailFastException(NULL, NULL, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);	// Calling abort() will generate a dialog box,
																						// but we already have our own. Raising a fail-fast
																						// exception skips it but also allows WER to do its
																						// job.
			break;
		default:
			throw std::invalid_argument("level was not one of known values");
		}

		return false;
	}
	else
	{
		return true;
	}
}

std::wstring Error::ExceptionFromHRESULT(HRESULT result)
{
	AutoFree::SilentLocal<wchar_t[]> error;
	const DWORD count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr, result, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), reinterpret_cast<wchar_t *>(error.put()), 0, nullptr);
	std::wostringstream stream;
	stream << L"Exception from HRESULT: " << (count ? Util::Trim<std::wstring_view>(error.get()) : L"[failed to get error message for HRESULT]") <<
		L" (0x" << std::setw(sizeof(HRESULT) * 2) << std::setfill(L'0') << std::hex << result << L')';
	return stream.str();
}