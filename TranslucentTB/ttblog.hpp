#pragma once
#ifndef TTBLOG_HPP
#define TTBLOG_HPP

#include <ctime>
#include <cwchar>
#include <fstream>
#include <string>

class Logger {

public:
	Logger(const std::wstring &file_path);
	void Log(const std::wstring &message);
	~Logger();

private:
	std::wofstream *log_stream;

};


namespace Log {
	static ::Logger *Instance;
	static std::wstring File;

	void OutputMessage(const std::wstring &message);
}



Logger::Logger(const std::wstring &file_path)
{
	log_stream = new std::wofstream(file_path);
}

void Logger::Log(const std::wstring &message)
{
	*log_stream << message << std::endl;
}

Logger::~Logger()
{
	log_stream->flush();
	log_stream->close();
	delete log_stream;
}


void Log::OutputMessage(const std::wstring &message)
{
	static std::wstring messages_before_init;

	std::time_t current_time = std::time(0);

	std::wstring buffer;
	buffer += '(';
	buffer += _wctime(&current_time);
	buffer = buffer.substr(0, buffer.length() - 1);
	buffer += L") ";
	buffer += message;

	if (Instance)
	{
		if (!messages_before_init.empty())
		{
			// Remove last new line
			messages_before_init = messages_before_init.substr(0, messages_before_init.length() - 1);
			Instance->Log(messages_before_init);
			messages_before_init.clear();
		}
		Instance->Log(buffer);
	}
	else
	{
		messages_before_init += buffer;
		messages_before_init += '\n';
	}

	OutputDebugString((message + L"\n").c_str());
}

#endif // !TTBLOG_HPP
