#include "ttberror.hpp"
#include <comdef.h>
#include <exception>
#include <winerror.h>
#include <WinUser.h>

#include "app.hpp"
#include "ttblog.hpp"

bool Error::Handle(const HRESULT &error, const Level &level, const std::wstring &message, const std::wstring &file, int line, const std::wstring &function)
{
	if (FAILED(error))
	{
		std::wstring boxbuffer;
		if (level != Level::Log)
		{
			boxbuffer += message + L"\n\n";

			if (level == Level::Fatal)
			{
				boxbuffer += L"Program will exit.\n\n";
			}

			boxbuffer += L"Exception from HRESULT: ";
			boxbuffer += _com_error(error).ErrorMessage();
		}

		Log::OutputMessage(message + L" Exception from HRESULT: " + _com_error(error).ErrorMessage() +
			L" (" + file + L":" + std::to_wstring(line) + L" at function " + function + L")");

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
