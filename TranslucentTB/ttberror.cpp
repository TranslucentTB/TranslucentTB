#include "ttberror.hpp"
#include <comdef.h>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <winerror.h>
#include <WinUser.h>

#include "common.hpp"
#include "ttblog.hpp"

bool Error::Handle(const HRESULT &error, const Level &level, const wchar_t *const message, const wchar_t *const file, const int &line, const char *const function)
{
	if (FAILED(error))
	{
		const std::wstring message_str(message);
		const std::wstring error_message = ExceptionFromHRESULT(error);
		std::wstring boxbuffer;
		if (level != Level::Log && level != Level::Debug)
		{
			boxbuffer += message_str;
			boxbuffer += L"\n\n";

			if (level == Level::Fatal)
			{
				boxbuffer += L"Program will exit.\n\n";
			}

			boxbuffer += error_message;
		}

		const size_t functionLength = std::char_traits<char>::length(function);
		std::wstring functionW;
		functionW.resize(functionLength);
		if (!MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, function, functionLength, functionW.data(), functionLength))
		{
			functionW = L"[failed to convert function name to UTF-16]";
		}

		const std::wstring err =
			message_str + L' ' + error_message +
			L" (" + file + L':' + std::to_wstring(line) + L" at function " + functionW + L')';

		switch (level)
		{
		case Level::Debug:
			OutputDebugString((err + L'\n').c_str());
			break;
		case Level::Log:
			Log::OutputMessage(err);
			break;
		case Level::Error:
			Log::OutputMessage(err);
			MessageBox(NULL, boxbuffer.c_str(), NAME L" - Error", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
			break;
		case Level::Fatal:
			Log::OutputMessage(err);
			MessageBox(NULL, boxbuffer.c_str(), NAME L" - Fatal error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
			std::terminate();
			break;
		default:
			throw std::invalid_argument("level was not one of known values");
			break;
		}

		return false;
	}
	else
	{
		return true;
	}
}

std::wstring Error::ExceptionFromHRESULT(const HRESULT &result)
{
	std::wostringstream stream;
	stream << L"Exception from HRESULT: " << _com_error(result).ErrorMessage() <<
		L" (0x" << std::setw(sizeof(HRESULT) * 2) << std::setfill(L'0') << std::hex << result << L')';
	return stream.str();
}