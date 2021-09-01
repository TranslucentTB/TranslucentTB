#pragma once
#include "arch.h"
#include <debugapi.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>

class debug_sink final : public spdlog::sinks::base_sink<spdlog::details::null_mutex> {
protected:
	void sink_it_(const spdlog::details::log_msg &msg) override
	{
		if (IsDebuggerPresent())
		{
			spdlog::memory_buf_t formatted;
			this->formatter_->format(msg, formatted);
			formatted.push_back('\0');

			OutputDebugStringA(formatted.data());
		}
	}

	void flush_() noexcept override
	{ }
};
