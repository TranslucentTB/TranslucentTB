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

namespace Error {

	enum class Level {
		Fatal,
		Error,
		Log
	};

	void Handle(const HRESULT error, const Level level, const std::wstring message)
	{
		if (FAILED(error))
		{
			std::wstring buffer;
			buffer += message;

			if (level == Level::Log)
			{
				buffer += ' ';
			}
			else
			{
				buffer += L"\n\n";
			}

			if (level == Level::Fatal)
			{
				buffer += L"Program will exit.\n\n";
			}

			buffer += L"Exception from HRESULT: ";
			buffer += _com_error(error).ErrorMessage();

			if (level == Level::Log)
			{
				buffer += '\n';
			}

			switch (level)
			{
				case Level::Fatal:
				{
					MessageBox(NULL, buffer.c_str(), (std::wstring(App::NAME) + L" - Fatal error").c_str(), MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
					std::terminate();
					break;
				}

				case Level::Error:
				{
					MessageBox(NULL, buffer.c_str(), (std::wstring(App::NAME) + L" - Error").c_str(), MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
				}

				case Level::Log:
				{
					OutputDebugString(buffer.c_str());
					break;
				}
			}
		}
	}
}

#endif // !TTBERROR_HPP
