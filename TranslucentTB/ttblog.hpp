#pragma once
#ifndef TTBLOG_HPP
#define TTBLOG_HPP

#include <ctime>
#include <fstream>
#include <string>

class Logger {

public:
	Logger(std::wstring file_path)
	{
		log_stream = new std::wofstream(file_path);
	}

	void Log(std::wstring message)
	{
		*log_stream << message << std::endl;
	}

	~Logger()
	{
		log_stream->flush();
		log_stream->close();
		delete log_stream;
	}


private:
	std::wofstream *log_stream;

};

namespace Log {
	static ::Logger *Instance;

	void OutputMessage(std::wstring message)
	{
		if (Instance)
		{
			std::time_t current_time = std::time(0);

			std::wstring buffer;
			buffer += '(';
			buffer += _wctime(&current_time);
			buffer += L") ";
			buffer += message;

			Instance->Log(buffer);
		}
	}
}

#endif // !TTBLOG_HPP
