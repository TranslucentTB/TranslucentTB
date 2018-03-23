#pragma once
#define _X86_
#include <string>
#include <windef.h>

class Error {

public:
	enum class Level {
		Fatal,	// Show an error message to the user and immediatly exit
		Error,	// Show an error message to the user and log to debug output
		Log		// Log to debug output
	};

	static bool Handle(const HRESULT &error, const Level &level, const std::wstring &message, const std::string &file, int line, const std::string &function);
};

#define ErrorHandle(x, y, z) (Error::Handle(x, y, z, __FILE__, __LINE__, __func__))