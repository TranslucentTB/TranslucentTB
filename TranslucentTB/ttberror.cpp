#include "ttberror.hpp"
#include <comdef.h>
#include <string>
#include <exception>
#include <sstream>
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

		std::wstring functionW;

		const size_t functionLength = std::char_traits<char>::length(function) + 1;
		std::vector<wchar_t> functionWtemp(functionLength);
		if (MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, function, functionLength, functionWtemp.data(), functionLength))
		{
			functionW = functionWtemp.data();
		}

		const std::wstring error =
			message_str + L' ' + error_message +
			L" (" + file + L':' + std::to_wstring(line) + L" at function " +
			(functionW.empty() ? L"[failed to convert function name to UTF-16]" : functionW) + L')';

		switch (level)
		{
		case Level::Debug:
			OutputDebugString((error + L'\n').c_str());
			break;
		case Level::Log:
			Log::OutputMessage(error);
			break;
		case Level::Error:
			MessageBox(NULL, boxbuffer.c_str(), (std::wstring(NAME) + L" - Error").c_str(), MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
			break;
		case Level::Fatal:
			MessageBox(NULL, boxbuffer.c_str(), (std::wstring(NAME) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
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
	stream << L"Exception from HRESULT: " << _com_error(result).ErrorMessage() << L" (0x" << reinterpret_cast<const void *>(result) << L')';
	return stream.str();
}
