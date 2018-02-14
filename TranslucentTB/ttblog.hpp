#pragma once
#ifndef TTBLOG_HPP
#define TTBLOG_HPP

#include <ctime>
#include <cwchar>
#include <fstream>
#include <string>

class Logger {

public:
	Logger(std::wstring file_path);
	void Log(std::wstring message);
	~Logger();

private:
	std::wofstream *log_stream;

};

namespace Log {
	static ::Logger *Instance;
	static std::wstring File;

	void OutputMessage(std::wstring message);
}



Logger::Logger(std::wstring file_path)
{
	log_stream = new std::wofstream(file_path);
	Log::File = file_path;
}

void Logger::Log(std::wstring message)
{
	*log_stream << message << std::endl;
}

Logger::~Logger()
{
	log_stream->flush();
	log_stream->close();
	delete log_stream;
}


void Log::OutputMessage(std::wstring message)
{
	if (Instance)
	{
		std::time_t current_time = std::time(0);

		std::wstring buffer;
		buffer += '(';
		buffer += _wctime(&current_time);
		buffer = buffer.substr(0, buffer.length() - 1);
		buffer += L") ";
		buffer += message;

		Instance->Log(buffer);
	}
}

#endif // !TTBLOG_HPP
