#include "ttberror.hpp"
#include <codecvt>
#include <comdef.h>
#include <locale>
#include <string>
#include <exception>
#include <winerror.h>
#include <WinUser.h>

#include "app.hpp"
#include "ttblog.hpp"

bool Error::Handle(const HRESULT &error, const Level &level, const wchar_t *const message, const wchar_t *const file, const int &line, const char *const function)
{
	if (FAILED(error))
	{
		std::wstring message_str(message);
		std::wstring error_message(_com_error(error).ErrorMessage());
		std::wstring boxbuffer;
		if (level != Level::Log)
		{
			boxbuffer += message_str + L"\n\n";

			if (level == Level::Fatal)
			{
				boxbuffer += L"Program will exit.\n\n";
			}

			boxbuffer += L"Exception from HRESULT: ";
			boxbuffer += error_message;
		}

		std::wstring functionW(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(function));

		Log::OutputMessage(message_str + L" Exception from HRESULT: " + error_message +
			L" (" + file + L":" + std::to_wstring(line) + L" at function " + functionW + L")");

		if (level == Level::Fatal)
		{
			MessageBox(NULL, boxbuffer.c_str(), (App::NAME + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
			std::terminate();
		}
		else if (level == Level::Error)
		{
			MessageBox(NULL, boxbuffer.c_str(), (App::NAME + L" - Error").c_str(), MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
		}

		return false;
	}
	else
	{
		return true;
	}
}
