#pragma once
#ifndef TTBERROR_HPP
#define TTBERROR_HPP

#include <comdef.h>
#include <exception>
#include <string>
#include <windef.h>
#include <winerror.h>
#include <winuser.h>

#include "app.hpp"
#include "ttblog.hpp"

namespace Error {

	enum class Level {
		Fatal,	// Show an error message to the user and immediatly exit
		Error,	// Show an error message to the user and log to debug output
		Log		// Log to debug output
	};

	bool Handle(const HRESULT error, const Level level, const std::wstring message)
	{
		if (FAILED(error))
		{
			std::wstring boxbuffer;
			if (level != Level::Log)
			{
				boxbuffer += message;
				boxbuffer += L"\n\n";

				if (level == Level::Fatal)
				{
					boxbuffer += L"Program will exit.\n\n";
				}

				boxbuffer += L"Exception from HRESULT: ";
				boxbuffer += _com_error(error).ErrorMessage();
			}

			std::wstring logbuffer;
			logbuffer += message;
			logbuffer += ' ';
			logbuffer += L"Exception from HRESULT: ";
			logbuffer += _com_error(error).ErrorMessage();
			logbuffer += '\n';
			Log::OutputMessage(logbuffer);

			switch (level)
			{
				case Level::Fatal:
				{
					MessageBox(NULL, boxbuffer.c_str(), (std::wstring(App::NAME) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
					std::terminate();
					break;
				}

				case Level::Error:
				{
					MessageBox(NULL, boxbuffer.c_str(), (std::wstring(App::NAME) + L" - Error").c_str(), MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
				}

				case Level::Log:
				{
					OutputDebugString(logbuffer.c_str());
					break;
				}
			}

			return false;
		}
		else
		{
			return true;
		}
	}
}

#endif // !TTBERROR_HPP
